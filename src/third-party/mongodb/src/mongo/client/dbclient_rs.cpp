// dbclient.cpp - connect to a Mongo database as a database, from C++

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "pch.h"

#include "mongo/client/dbclient_rs.h"

#include <fstream>

#include "mongo/base/init.h"
#include "mongo/bson/util/builder.h"
#include "mongo/client/connpool.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/db/dbmessage.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/json.h"
#include "mongo/util/background.h"
#include "mongo/util/scopeguard.h"
#include "mongo/util/timer.h"

namespace mongo {
    /*
     * Set of commands that can be used with $readPreference
     */
    set<string> _secOkCmdList;

    MONGO_INITIALIZER(PopulateReadPrefSecOkCmdList)(::mongo::InitializerContext* context) {
        _secOkCmdList.insert("aggregate");
        _secOkCmdList.insert("collStats");
        _secOkCmdList.insert("count");
        _secOkCmdList.insert("distinct");
        _secOkCmdList.insert("dbStats");
        _secOkCmdList.insert("geoNear");
        _secOkCmdList.insert("geoSearch");
        _secOkCmdList.insert("geoWalk");
        _secOkCmdList.insert("group");

        return Status::OK();
    }

    /**
     * @param ns the namespace of the query.
     * @param queryOptionFlags the flags for the query.
     * @param queryObj the query object to check.
     *
     * @return true if the given query can be sent to a secondary node without taking the
     *     slaveOk flag into account.
     */
    bool _isQueryOkToSecondary(const string& ns, int queryOptionFlags, const BSONObj& queryObj) {
        if (queryOptionFlags & QueryOption_SlaveOk) {
            return true;
        }

        // _secOkCmdList was not initialized! mongo::runGlobalInitializersOrDie
        // probably was not called.
        fassert(16464, !_secOkCmdList.empty());

        if (!hasReadPreference(queryObj)) {
            return false;
        }

        if (ns.find(".$cmd") == string::npos) {
            return true;
        }

        BSONObj actualQueryObj;
        if (strcmp(queryObj.firstElement().fieldName(), "query") == 0) {
            actualQueryObj = queryObj["query"].embeddedObject();
        }
        else {
            actualQueryObj = queryObj;
        }

        const string cmdName = actualQueryObj.firstElementFieldName();
        if (_secOkCmdList.count(cmdName) == 1) {
            return true;
        }

        if (cmdName == "mapReduce" || cmdName == "mapreduce") {
            if (!actualQueryObj.hasField("out")) {
                return false;
            }

            BSONElement outElem(actualQueryObj["out"]);
            if (outElem.isABSONObj() && outElem["inline"].trueValue()) {
                return true;
            }
        }

        return false;
    }

    /**
     * Selects the right node given the nodes to pick from and the preference.
     * This method does strict tag matching, and will not implicitly fallback
     * to matching anything.
     *
     * @param nodes the nodes to select from
     * @param readPreferenceTag the tags to use for choosing the right node
     * @param secOnly never select a primary if true
     * @param localThresholdMillis the exclusive upper bound of ping time to be
     *     considered as a local node. Local nodes are favored over non-local
     *     nodes if multiple nodes matches the other criteria.
     * @param lastHost the last host returned (mainly used for doing round-robin).
     *     Will be overwritten with the newly returned host if not empty. Should
     *     never be NULL.
     * @param isPrimarySelected out parameter that is set to true if the returned host
     *     is a primary.
     *
     * @return the host object of the node selected. If none of the nodes are
     *     eligible, returns an empty host. Cannot be NULL and valid only if returned
     *     host is not empty.
     */
    HostAndPort _selectNode(const vector<ReplicaSetMonitor::Node>& nodes,
                            const BSONObj& readPreferenceTag,
                            bool secOnly,
                            int localThresholdMillis,
                            HostAndPort* lastHost /* in/out */,
                            bool* isPrimarySelected) {
        HostAndPort fallbackHost;

        // Implicit: start from index 0 if lastHost doesn't exist anymore
        size_t nextNodeIndex = 0;

        if (!lastHost->empty()) {
            for (size_t x = 0; x < nodes.size(); x++) {
                if (*lastHost == nodes[x].addr) {
                    nextNodeIndex = x;
                    break;
                }
            }
        }

        for (size_t itNode = 0; itNode < nodes.size(); ++itNode) {
            nextNodeIndex = (nextNodeIndex + 1) % nodes.size();
            const ReplicaSetMonitor::Node& node = nodes[nextNodeIndex];

            if (!node.ok) {
                LOG(2) << "dbclient_rs not selecting " << node << ", not currently ok" << endl;
                continue;
            }

            if (secOnly && !node.okForSecondaryQueries()) {
                continue;
            }

            if (node.matchesTag(readPreferenceTag)) {
                // found an ok candidate; may not be local.
                fallbackHost = node.addr;
                *isPrimarySelected = node.ismaster;

                if (node.isLocalSecondary(localThresholdMillis)) {
                    // found a local node.  return early.
                    LOG(2) << "dbclient_rs _selectNode found local secondary for queries: "
                           << nextNodeIndex << ", ping time: " << node.pingTimeMillis << endl;
                    *lastHost = fallbackHost;
                    return fallbackHost;
                }
            }
        }

        if (!fallbackHost.empty()) {
            *lastHost = fallbackHost;
        }

        return fallbackHost;
    }

    /**
     * Extracts the read preference settings from the query document. Note that this method
     * assumes that the query is ok for secondaries so it defaults to
     * ReadPreference_SecondaryPreferred when nothing is specified. Supports the following
     * format:
     *
     * Format A (official format):
     * { query: <actual query>, $readPreference: <read pref obj> }
     *
     * Format B (unofficial internal format from mongos):
     * { <actual query>, $queryOptions: { $readPreference: <read pref obj> }}
     *
     * @param query the raw query document
     * @param pref an out parameter and will contain the read preference mode extracted
     *      from the query object.
     *
     * @return the tag set list. If the tags field was not present, it will contain one
     *      empty tag document {} which matches any tag. Caller owns the TagSet object
     *      and is responsible for deletion.
     *
     * @throws AssertionException if the read preference object is malformed
     */
    TagSet* _extractReadPref(const BSONObj& query, ReadPreference* pref) {
        if (!hasReadPreference(query)) {
            *pref = mongo::ReadPreference_SecondaryPreferred;
        }
        else {
            BSONElement readPrefElement;

            if (query.hasField("$readPreference")) {
                readPrefElement = query["$readPreference"];
            }
            else {
                readPrefElement = query["$queryOptions"]["$readPreference"];
            }

            uassert(16381, "$readPreference should be an object",
                    readPrefElement.isABSONObj());
            const BSONObj& prefDoc = readPrefElement.Obj();

            uassert(16382, "mode not specified for read preference", prefDoc.hasField("mode"));

            const string mode = prefDoc["mode"].String();

            if (mode == "primary") {
                *pref = mongo::ReadPreference_PrimaryOnly;
            }
            else if (mode == "primaryPreferred") {
                *pref = mongo::ReadPreference_PrimaryPreferred;
            }
            else if (mode == "secondary") {
                *pref = mongo::ReadPreference_SecondaryOnly;
            }
            else if (mode == "secondaryPreferred") {
                *pref = mongo::ReadPreference_SecondaryPreferred;
            }
            else if (mode == "nearest") {
                *pref = mongo::ReadPreference_Nearest;
            }
            else {
                uasserted(16383, str::stream() << "Unknown read preference mode: " << mode);
            }

            if (prefDoc.hasField("tags")) {
                uassert(16384, "Cannot specify tags for primary only read preference",
                        *pref != mongo::ReadPreference_PrimaryOnly);

                const BSONElement& tagsElem = prefDoc["tags"];
                uassert(16385, "tags for read preference should be an array",
                        tagsElem.type() == mongo::Array);

                return new TagSet(BSONArray(tagsElem.Obj()));
            }
        }

        BSONArrayBuilder arrayBuilder;
        arrayBuilder.append(BSONObj());
        return new TagSet(arrayBuilder.arr());
    }

    /**
     * @return the connection associated with the monitor node. Will also attempt
     *     to establish connection if NULL. Can still return NULL if reconnect failed.
     */
    shared_ptr<DBClientConnection> _getConnWithRefresh(ReplicaSetMonitor::Node& node) {
        if (node.conn.get() == NULL) {
            ConnectionString connStr(node.addr);
            string errmsg;

            try {
                node.conn.reset(dynamic_cast<DBClientConnection*>(
                        connStr.connect(errmsg, ReplicaSetMonitor::SOCKET_TIMEOUT_SECS)));
            }
            catch (const AssertionException&) {
                node.ok = false;
            }
        }

        return node.conn;
    }

    // --------------------------------
    // ----- ReplicaSetMonitor ---------
    // --------------------------------

    // global background job responsible for checking every X amount of time
    class ReplicaSetMonitorWatcher : public BackgroundJob {
    public:
        ReplicaSetMonitorWatcher() : _safego("ReplicaSetMonitorWatcher::_safego") , _started(false) {}

        virtual string name() const { return "ReplicaSetMonitorWatcher"; }
        
        void safeGo() {
            // check outside of lock for speed
            if ( _started )
                return;
            
            scoped_lock lk( _safego );
            if ( _started )
                return;
            _started = true;

            go();
        }
    protected:
        void run() {
            log() << "starting" << endl;
            while ( ! inShutdown() ) {
                sleepsecs( 10 );
                try {
                    ReplicaSetMonitor::checkAll( true );
                }
                catch ( std::exception& e ) {
                    error() << "check failed: " << e.what() << endl;
                }
                catch ( ... ) {
                    error() << "unkown error" << endl;
                }
            }
        }

        mongo::mutex _safego;
        bool _started;

    } replicaSetMonitorWatcher;

    string seedString( const vector<HostAndPort>& servers ){
        string seedStr;
        for ( unsigned i = 0; i < servers.size(); i++ ){
            seedStr += servers[i].toString();
            if( i < servers.size() - 1 ) seedStr += ",";
        }

        return seedStr;
    }

    const double ReplicaSetMonitor::SOCKET_TIMEOUT_SECS = 5;

    // Must already be in _setsLock when constructing a new ReplicaSetMonitor. This is why you
    // should only create ReplicaSetMonitors from ReplicaSetMonitor::get and
    // ReplicaSetMonitor::createIfNeeded.
    ReplicaSetMonitor::ReplicaSetMonitor( const string& name , const vector<HostAndPort>& servers )
        : _lock( "ReplicaSetMonitor instance" ),
          _checkConnectionLock( "ReplicaSetMonitor check connection lock" ),
          _name( name ), _master(-1),
          _nextSlave(0), _failedChecks(0), _localThresholdMillis(cmdLine.defaultLocalThresholdMillis) {

        uassert( 13642 , "need at least 1 node for a replica set" , servers.size() > 0 );

        if ( _name.size() == 0 ) {
            warning() << "replica set name empty, first node: " << servers[0] << endl;
        }

        log() << "starting new replica set monitor for replica set " << _name << " with seed of " << seedString( servers ) << endl;
        _populateHosts_inSetsLock(servers);

        _seedServers.insert( pair<string, vector<HostAndPort> >(name, servers) );

        log() << "replica set monitor for replica set " << _name << " started, address is " << getServerAddress() << endl;

    }

    // Must already be in _setsLock when destroying a ReplicaSetMonitor. This is why you should only
    // delete ReplicaSetMonitors from ReplicaSetMonitor::remove.
    ReplicaSetMonitor::~ReplicaSetMonitor() {
        scoped_lock lk ( _lock );
        log() << "deleting replica set monitor for: " << _getServerAddress_inlock() << endl;
        _cacheServerAddresses_inlock();
        pool.removeHost( _getServerAddress_inlock() );
        _nodes.clear();
        _master = -1;
    }

    void ReplicaSetMonitor::_cacheServerAddresses_inlock() {
        // Save list of current set members so that the monitor can be rebuilt if needed.
        vector<HostAndPort>& servers = _seedServers[_name];
        servers.clear();
        for ( vector<Node>::iterator it = _nodes.begin(); it < _nodes.end(); ++it ) {
            servers.push_back( it->addr );
        }
    }

    void ReplicaSetMonitor::createIfNeeded( const string& name , const vector<HostAndPort>& servers ) {
        scoped_lock lk( _setsLock );
        ReplicaSetMonitorPtr& m = _sets[name];
        if ( ! m )
            m.reset( new ReplicaSetMonitor( name , servers ) );

        replicaSetMonitorWatcher.safeGo();
    }

    ReplicaSetMonitorPtr ReplicaSetMonitor::get( const string& name , const bool createFromSeed ) {
        scoped_lock lk( _setsLock );
        map<string,ReplicaSetMonitorPtr>::const_iterator i = _sets.find( name );
        if ( i != _sets.end() ) {
            return i->second;
        }
        if ( createFromSeed ) {
            map<string,vector<HostAndPort> >::const_iterator j = _seedServers.find( name );
            if ( j != _seedServers.end() ) {
                LOG(4) << "Creating ReplicaSetMonitor from cached address" << endl;
                ReplicaSetMonitorPtr& m = _sets[name];
                verify( !m );
                m.reset( new ReplicaSetMonitor( name, j->second ) );
                replicaSetMonitorWatcher.safeGo();
                return m;
            }
        }
        return ReplicaSetMonitorPtr();
    }

    void ReplicaSetMonitor::getAllTrackedSets(set<string>* activeSets) {
        scoped_lock lk( _setsLock );
        for (map<string,ReplicaSetMonitorPtr>::const_iterator it = _sets.begin();
             it != _sets.end(); ++it)
        {
            activeSets->insert(it->first);
        }
    }

    void ReplicaSetMonitor::checkAll( bool checkAllSecondaries ) {
        set<string> seen;

        while ( true ) {
            ReplicaSetMonitorPtr m;
            {
                scoped_lock lk( _setsLock );
                for ( map<string,ReplicaSetMonitorPtr>::iterator i=_sets.begin(); i!=_sets.end(); ++i ) {
                    string name = i->first;
                    if ( seen.count( name ) )
                        continue;
                    LOG(1) << "checking replica set: " << name << endl;
                    seen.insert( name );
                    m = i->second;
                    break;
                }
            }

            if ( ! m )
                break;

            m->check( checkAllSecondaries );
            {
                scoped_lock lk( _setsLock );
                if ( m->_failedChecks >= _maxFailedChecks ) {
                    log() << "Replica set " << m->getName() << " was down for " << m->_failedChecks
                          << " checks in a row. Stopping polled monitoring of the set." << endl;
                    _remove_inlock( m->getName() );
                }
            }
        }


    }

    void ReplicaSetMonitor::remove( const string& name, bool clearSeedCache ) {
        scoped_lock lk( _setsLock );
        _remove_inlock( name, clearSeedCache );
    }

    void ReplicaSetMonitor::_remove_inlock( const string& name, bool clearSeedCache ) {
        LOG(2) << "Removing ReplicaSetMonitor for " << name << " from replica set table" << endl;
        _sets.erase( name );
        if ( clearSeedCache ) {
            _seedServers.erase( name );
        }
    }

    void ReplicaSetMonitor::setConfigChangeHook( ConfigChangeHook hook ) {
        massert( 13610 , "ConfigChangeHook already specified" , _hook == 0 );
        _hook = hook;
    }

    void ReplicaSetMonitor::setLocalThresholdMillis( const int millis ) {
        scoped_lock lk( _lock );
        _localThresholdMillis = millis;
    }

    string ReplicaSetMonitor::getServerAddress() const {
        scoped_lock lk( _lock );
        return _getServerAddress_inlock();
    }

    string ReplicaSetMonitor::_getServerAddress_inlock() const {
        StringBuilder ss;
        if ( _name.size() )
            ss << _name << "/";

        for ( unsigned i=0; i<_nodes.size(); i++ ) {
            if ( i > 0 )
                ss << ",";
            _nodes[i].addr.append( ss );
        }

        return ss.str();
    }

    bool ReplicaSetMonitor::contains( const string& server ) const {
        scoped_lock lk( _lock );
        for ( unsigned i=0; i<_nodes.size(); i++ ) {
            if ( _nodes[i].addr == server )
                return true;
        }
        return false;
    }
    

    void ReplicaSetMonitor::notifyFailure( const HostAndPort& server ) {
        scoped_lock lk( _lock );
        
        if ( _master >= 0 && _master < (int)_nodes.size() ) {
            if ( server == _nodes[_master].addr ) {
                _nodes[_master].ok = false; 
                _master = -1;
            }
        }
    }



    HostAndPort ReplicaSetMonitor::getMaster() {
        {
            scoped_lock lk( _lock );
            verify(_master < static_cast<int>(_nodes.size()));
            if ( _master >= 0 && _nodes[_master].ok )
                return _nodes[_master].addr;
        }
        
        _check( false );

        scoped_lock lk( _lock );
        uassert( 10009 , str::stream() << "ReplicaSetMonitor no master found for set: " << _name , _master >= 0 );
        verify(_master < static_cast<int>(_nodes.size()));
        return _nodes[_master].addr;
    }
    
    HostAndPort ReplicaSetMonitor::getSlave( const HostAndPort& prev ) {
        // make sure its valid

        bool wasFound = false;
        bool wasMaster = false;

        // This is always true, since checked in port()
        verify( prev.port() >= 0 );
        if( prev.host().size() ){
            scoped_lock lk( _lock );
            for ( unsigned i=0; i<_nodes.size(); i++ ) {
                if ( prev != _nodes[i].addr ) 
                    continue;

                wasFound = true;

                if ( _nodes[i].okForSecondaryQueries() )
                    return prev;

                wasMaster = _nodes[i].ok && !_nodes[i].secondary;
                break;
            }
        }
        
        if( prev.host().size() ){
            if( wasFound ){ LOG(1) << "slave '" << prev << ( wasMaster ? "' is master node, trying to find another node" :
                                                                         "' is no longer ok to use" ) << endl; }
            else{ LOG(1) << "slave '" << prev << "' was not found in the replica set" << endl; }
        }
        else LOG(1) << "slave '" << prev << "' is not initialized or invalid" << endl;

        return getSlave();
    }

    HostAndPort ReplicaSetMonitor::getSlave( bool preferLocal ) {
        LOG(2) << "dbclient_rs getSlave " << getServerAddress() << endl;

        HostAndPort fallbackNode;
        scoped_lock lk( _lock );

        for ( size_t itNode = 0; itNode < _nodes.size(); ++itNode ) {
            _nextSlave = ( _nextSlave + 1 ) % _nodes.size();
            if ( _nextSlave != _master ) {
                if ( _nodes[ _nextSlave ].okForSecondaryQueries() ) {
                    // found an ok slave; may not be local.
                    fallbackNode = _nodes[ _nextSlave ].addr;
                    if ( ! preferLocal )
                        return fallbackNode;
                    else if ( _nodes[ _nextSlave ].isLocalSecondary( _localThresholdMillis ) ) {
                        // found a local slave.  return early.
                        LOG(2) << "dbclient_rs getSlave found local secondary for queries: "
                               << _nextSlave << ", ping time: "
                               << _nodes[ _nextSlave ].pingTimeMillis << endl;
                        return fallbackNode;
                    }
                }
                else
                    LOG(2) << "dbclient_rs getSlave not selecting " << _nodes[_nextSlave]
                           << ", not currently okForSecondaryQueries" << endl;
            }
        }

        if ( ! fallbackNode.empty() ) {
            // use a non-local secondary, even if local was preferred
            LOG(1) << "dbclient_rs getSlave falling back to a non-local secondary node" << endl;
            return fallbackNode;
        }

        massert(15899, str::stream() << "No suitable secondary found for slaveOk query"
                "in replica set: " << _name, _master >= 0 &&
                _master < static_cast<int>(_nodes.size()) && _nodes[_master].ok);

        // Fall back to primary
        LOG(1) << "dbclient_rs getSlave no member in secondary state found, "
                  "returning primary " << _nodes[ _master ] << endl;
        return _nodes[_master].addr;
    }

    /**
     * notify the monitor that server has failed
     */
    void ReplicaSetMonitor::notifySlaveFailure( const HostAndPort& server ) {
        scoped_lock lk( _lock );
        int x = _find_inlock( server );
        if ( x >= 0 ) {
            _nodes[x].ok = false;
        }
    }

    void ReplicaSetMonitor::_checkStatus( const string& hostAddr ) {
        BSONObj status;

        /* replSetGetStatus requires admin auth so use a connection from the pool,
         * and tell it to use the internal credentials.
         */
        scoped_ptr<ScopedDbConnection> authenticatedConn(
                ScopedDbConnection::getInternalScopedDbConnection( hostAddr ) );

        if ( !authenticatedConn->get()->runCommand( "admin",
                                                    BSON( "replSetGetStatus" << 1 ),
                                                    status )) {
            LOG(1) << "dbclient_rs replSetGetStatus failed" << status << endl;
            authenticatedConn->done(); // connection worked properly, but we got an error from server
            return;
        }

        // Make sure we return when finished
        authenticatedConn->done();

        if( !status.hasField("members") ) { 
            log() << "dbclient_rs error expected members field in replSetGetStatus result" << endl;
            return;
        }
        if( status["members"].type() != Array) {
            log() << "dbclient_rs error expected members field in replSetGetStatus result to be an array" << endl;
            return;
        }

        BSONObjIterator hi(status["members"].Obj());
        while (hi.more()) {
            BSONObj member = hi.next().Obj();
            string host = member["name"].String();

            int m = -1;
            if ((m = _find(host)) < 0) {
                LOG(1) << "dbclient_rs _checkStatus couldn't _find(" << host << ')' << endl;
                continue;
            }

            double state = member["state"].Number();
            if (member["health"].Number() == 1 && (state == 1 || state == 2)) {
                LOG(1) << "dbclient_rs nodes["<<m<<"].ok = true " << host << endl;
                scoped_lock lk( _lock );
                _nodes[m].ok = true;
            }
            else {
                LOG(1) << "dbclient_rs nodes["<<m<<"].ok = false " << host << endl;
                scoped_lock lk( _lock );
                _nodes[m].ok = false;
            }
        }
    }

    NodeDiff ReplicaSetMonitor::_getHostDiff_inlock( const BSONObj& hostList ){

        NodeDiff diff;
        set<int> nodesFound;

        int index = 0;
        BSONObjIterator hi( hostList );
        while( hi.more() ){

            string toCheck = hi.next().String();
            int nodeIndex = _find_inlock( toCheck );

            // Node-to-add
            if( nodeIndex < 0 ) diff.first.insert( toCheck );
            else nodesFound.insert( nodeIndex );

            index++;
        }

        for( size_t i = 0; i < _nodes.size(); i++ ){
            if( nodesFound.find( static_cast<int>(i) ) == nodesFound.end() ) diff.second.insert( static_cast<int>(i) );
        }

        return diff;
    }

    bool ReplicaSetMonitor::_shouldChangeHosts( const BSONObj& hostList, bool inlock ){

        int origHosts = 0;
        if( ! inlock ){
            scoped_lock lk( _lock );
            origHosts = _nodes.size();
        }
        else origHosts = _nodes.size();
        int numHosts = 0;
        bool changed = false;

        BSONObjIterator hi(hostList);
        while ( hi.more() ) {
            string toCheck = hi.next().String();

            numHosts++;
            int index = 0;
            if( ! inlock ) index = _find( toCheck );
            else index = _find_inlock( toCheck );

            if ( index >= 0 ) continue;

            changed = true;
            break;
        }

        return (changed || origHosts != numHosts) && numHosts > 0;

    }

    void ReplicaSetMonitor::_checkHosts( const BSONObj& hostList, bool& changed ) {

        // Fast path, still requires intermittent locking
        if( ! _shouldChangeHosts( hostList, false ) ){
            changed = false;
            return;
        }

        // Slow path, double-checked though
        scoped_lock lk( _lock );

        // Our host list may have changed while waiting for another thread in the meantime,
        // so double-check here
        // TODO:  Do we really need this much protection, this should be pretty rare and not
        // triggered from lots of threads, duping old behavior for safety
        if( ! _shouldChangeHosts( hostList, true ) ){
            changed = false;
            return;
        }

        // LogLevel can be pretty low, since replica set reconfiguration should be pretty rare and
        // we want to record our changes
        log() << "changing hosts to " << hostList << " from " << _getServerAddress_inlock() << endl;

        NodeDiff diff = _getHostDiff_inlock( hostList );
        set<string> added = diff.first;
        set<int> removed = diff.second;

        verify( added.size() > 0 || removed.size() > 0 );
        changed = true;

        // Delete from the end so we don't invalidate as we delete, delete indices are ascending
        for( set<int>::reverse_iterator i = removed.rbegin(), end = removed.rend(); i != end; ++i ){

            log() << "erasing host " << _nodes[ *i ] << " from replica set " << this->_name << endl;
            _nodes.erase( _nodes.begin() + *i );
        }

        // Add new nodes
        for( set<string>::iterator i = added.begin(), end = added.end(); i != end; ++i ){

            log() << "trying to add new host " << *i << " to replica set " << this->_name << endl;

            // Connect to new node
            HostAndPort host(*i);
            ConnectionString connStr(host);

            uassert(16530, str::stream() << "cannot create a replSet node connection that "
                    "is not single: " << host.toString(true),
                    connStr.type() == ConnectionString::MASTER ||
                    connStr.type() == ConnectionString::CUSTOM);

            DBClientConnection* newConn = NULL;
            string errmsg;
            try {
                // Needs to perform a dynamic_cast because we need to set the replSet
                // callback. We should eventually not need this after we remove the
                // callback.
                newConn = dynamic_cast<DBClientConnection*>(
                        connStr.connect(errmsg, SOCKET_TIMEOUT_SECS));
            }
            catch (const AssertionException& ex) {
                errmsg = ex.toString();
            }

            if (errmsg.empty()) {
                log() << "successfully connected to new host " << *i
                        << " in replica set " << this->_name << endl;
            }
            else {
                warning() << "cannot connect to new host " << *i
                        << " to replica set " << this->_name
                        << ", err: " << errmsg << endl;
            }

            _nodes.push_back(Node(host, newConn));
        }

        // Invalidate the cached _master index since the _nodes structure has
        // already been modified.
        _master = -1;
    }
    

    bool ReplicaSetMonitor::_checkConnection( DBClientConnection* conn,
            string& maybePrimary, bool verbose, int nodesOffset ) {

        verify( conn );
        scoped_lock lk( _checkConnectionLock );
        bool isMaster = false;
        bool changed = false;
        bool errorOccured = false;

        if ( nodesOffset >= 0 ){
            scoped_lock lk( _lock );
            if ( !_checkConnMatch_inlock( conn, nodesOffset )) {
                /* Another thread modified _nodes -> invariant broken.
                 * This also implies that another thread just passed
                 * through here and refreshed _nodes. So no need to do
                 * duplicate work.
                 */
                return false;
            }
        }
        
        try {
            Timer t;
            BSONObj o;
            conn->isMaster( isMaster, &o );

            if ( o["setName"].type() != String || o["setName"].String() != _name ) {
                warning() << "node: " << conn->getServerAddress()
                          << " isn't a part of set: " << _name
                          << " ismaster: " << o << endl;

                if ( nodesOffset >= 0 ) {
                    scoped_lock lk( _lock );
                    _nodes[nodesOffset].ok = false;
                }

                return false;
            }
            int commandTime = t.millis();

            if ( nodesOffset >= 0 ) {
                scoped_lock lk( _lock );
                Node& node = _nodes[nodesOffset];

                if (node.pingTimeMillis == 0) {
                    node.pingTimeMillis = commandTime;
                }
                else {
                    // update ping time with smoothed moving averaged (1/4th the delta)
                    node.pingTimeMillis += (commandTime - node.pingTimeMillis) / 4;
                }

                node.hidden = o["hidden"].trueValue();
                node.secondary = o["secondary"].trueValue();
                node.ismaster = o["ismaster"].trueValue();

                node.lastIsMaster = o.copy();
            }

            LOG( verbose ? 0 : 1 ) << "ReplicaSetMonitor::_checkConnection: " << conn->toString()
                             << ' ' << o << endl;
            
            // add other nodes
            BSONArrayBuilder b;
            if ( o["hosts"].type() == Array ) {
                if ( o["primary"].type() == String )
                    maybePrimary = o["primary"].String();

                BSONObjIterator it( o["hosts"].Obj() );
                while( it.more() ) b.append( it.next() );
            }

            if (o.hasField("passives") && o["passives"].type() == Array) {
                BSONObjIterator it( o["passives"].Obj() );
                while( it.more() ) b.append( it.next() );
            }
            
            _checkHosts( b.arr(), changed);
            _checkStatus( conn->getServerAddress() );

        }
        catch ( std::exception& e ) {
            LOG( verbose ? 0 : 1 ) << "ReplicaSetMonitor::_checkConnection: caught exception "
                             << conn->toString() << ' ' << e.what() << endl;

            errorOccured = true;
        }

        if ( errorOccured && nodesOffset >= 0 ) {
            scoped_lock lk( _lock );

            if (_checkConnMatch_inlock(conn, nodesOffset)) {
                // Make sure _checkHosts didn't modify the _nodes structure
                _nodes[nodesOffset].ok = false;
            }
        }

        if ( changed && _hook )
            _hook( this );

        return isMaster;
    }

    void ReplicaSetMonitor::_check( bool checkAllSecondaries ) {
        LOG(1) <<  "_check : " << getServerAddress() << endl;

        int newMaster = -1;
        shared_ptr<DBClientConnection> nodeConn;

        for ( int retry = 0; retry < 2; retry++ ) {
            bool triedQuickCheck = false;

            if ( !checkAllSecondaries ) {
                scoped_lock lk( _lock );
                verify(_master < static_cast<int>(_nodes.size()));
                if ( _master >= 0 && _nodes[_master].ok ) {
                  /* Nothing else to do since another thread already
                   * found a usable _master
                   */
                  return;
                }
            }

            for ( unsigned i = 0; /* should not check while outside of lock! */ ; i++ ) {
                {
                    scoped_lock lk( _lock );
                    if ( i >= _nodes.size() ) break;
                    nodeConn = _getConnWithRefresh(_nodes[i]);
                    if (nodeConn.get() == NULL) continue;
                }

                string maybePrimary;
                if ( _checkConnection( nodeConn.get(), maybePrimary, retry, i ) ) {
                    scoped_lock lk( _lock );
                    if ( _checkConnMatch_inlock( nodeConn.get(), i )) {
                        newMaster = i;
                        if ( newMaster != _master ) {
                            log() << "Primary for replica set " << _name
                                  << " changed to " << _nodes[newMaster].addr << endl;
                        }
                        _master = i;

                        if ( !checkAllSecondaries )
                            return;
                    }
                    else {
                        /*
                         * Somebody modified _nodes and most likely set the new
                         * _master, so try again.
                         */
                        break;
                    }
                }


                if ( ! triedQuickCheck && ! maybePrimary.empty() ) {
                    int probablePrimaryIdx = -1;
                    shared_ptr<DBClientConnection> probablePrimaryConn;

                    {
                        scoped_lock lk( _lock );
                        probablePrimaryIdx = _find_inlock( maybePrimary );

                        if (probablePrimaryIdx >= 0) {
                            probablePrimaryConn = _getConnWithRefresh(
                                    _nodes[probablePrimaryIdx]);
                            if (probablePrimaryConn.get() == NULL) continue;
                        }
                    }

                    if ( probablePrimaryIdx >= 0 ) {
                        triedQuickCheck = true;

                        string dummy;
                        if ( _checkConnection( probablePrimaryConn.get(), dummy,
                                false, probablePrimaryIdx ) ) {

                            scoped_lock lk( _lock );

                            if ( _checkConnMatch_inlock( probablePrimaryConn.get(),
                                                         probablePrimaryIdx )) {
                              

                                newMaster = probablePrimaryIdx;
                                if ( newMaster != _master ) {
                                    log() << "Primary for replica set " << _name << " changed to " << _nodes[newMaster].addr << endl;
                                }
                                _master = probablePrimaryIdx;

                                if ( ! checkAllSecondaries )
                                    return;
                            }
                            else {
                                /*
                                 * Somebody modified _nodes and most likely set the
                                 * new _master, so try again.
                                 */
                                break;
                            }
                        }
                    }
                }
            }
            
            if ( newMaster >= 0 )
                return;

            sleepsecs( 1 );
        }

        {
            warning() << "No primary detected for set " << _name << endl;
            scoped_lock lk( _lock );
            _master = -1;

            for (vector<Node>::iterator iter = _nodes.begin(); iter < _nodes.end(); ++iter) {
                iter->ismaster = false;
            }

            if (checkAllSecondaries) {
                for ( unsigned i = 0; i < _nodes.size(); i++ ) {
                    if ( _nodes[i].ok ) {
                        _failedChecks = 0;
                        return;
                    }
                }
                // None of the nodes are ok.
                _failedChecks++;
                log() << "All nodes for set " << _name << " are down. This has happened for " << _failedChecks << " checks in a row. Polling will stop after " << _maxFailedChecks - _failedChecks << " more failed checks" << endl;
            }
        }
    }

    void ReplicaSetMonitor::check( bool checkAllSecondaries ) {
        bool isNodeEmpty = false;

        {
            scoped_lock lk( _lock );
            isNodeEmpty = _nodes.empty();
        }

        if (isNodeEmpty) {
            scoped_lock lk(_setsLock);
            _populateHosts_inSetsLock(_seedServers[_name]);
            /* _populateHosts_inlock already refreshes _nodes so no more work
             * needs to be done. If it was unsuccessful, the succeeding lines
             * will also fail, so no point in trying.
             */
            return;
        }

        shared_ptr<DBClientConnection> masterConn;

        {
            scoped_lock lk( _lock );

            // first see if the current master is fine
            if ( _master >= 0 ) {
                verify(_master < static_cast<int>(_nodes.size()));
                masterConn = _getConnWithRefresh(_nodes[_master]);
            }
        }

        if ( masterConn.get() != NULL ) {
            string temp;

            if ( _checkConnection( masterConn.get(), temp, false, _master )) {
                if ( ! checkAllSecondaries ) {
                    // current master is fine, so we're done
                    return;
                }
            }
        }

        // we either have no master, or the current is dead
        _check( checkAllSecondaries );
    }

    int ReplicaSetMonitor::_find( const string& server ) const {
        scoped_lock lk( _lock );
        return _find_inlock( server );
    }

    int ReplicaSetMonitor::_find_inlock( const string& server ) const {
        const size_t size = _nodes.size();

        for ( unsigned i = 0; i < size; i++ ) {
            if ( _nodes[i].addr == server ) {
                return i;
            }
        }

        return -1;
    }

    void ReplicaSetMonitor::appendInfo(BSONObjBuilder& bsonObjBuilder) const {
        scoped_lock lk(_lock);
        BSONArrayBuilder hosts(bsonObjBuilder.subarrayStart("hosts"));
        for (unsigned i = 0; i < _nodes.size(); i++) {
            const Node& node = _nodes[i];

            /* Note: cannot use toBSON helper method due to backwards compatibility.
             * In particular, toBSON method uses "isMaster" while this method
             * uses "ismaster"
             */
            BSONObjBuilder builder;
            builder.append("addr", node.addr.toString());
            builder.append("ok", node.ok);
            builder.append("ismaster", node.ismaster);
            builder.append("hidden", node.hidden);
            builder.append("secondary", node.secondary);
            builder.append("pingTimeMillis", node.pingTimeMillis);

            const BSONElement& tagElem = node.lastIsMaster["tags"];
            if (tagElem.ok() && tagElem.isABSONObj()) {
                builder.append("tags", tagElem.Obj());
            }

            hosts.append(builder.obj());
        }
        hosts.done();

        bsonObjBuilder.append("master", _master);
        bsonObjBuilder.append("nextSlave", _nextSlave);
    }
    
    bool ReplicaSetMonitor::_checkConnMatch_inlock( DBClientConnection* conn,
            size_t nodeOffset ) const {
        return (nodeOffset < _nodes.size() &&
                // Assumption: value for getServerAddress was extracted from
                // HostAndPort::toString()
                conn->getServerAddress() == _nodes[nodeOffset].addr.toString());
    }

    HostAndPort ReplicaSetMonitor::selectAndCheckNode(ReadPreference preference,
                                                      TagSet* tags,
                                                      bool* isPrimarySelected) {

        HostAndPort candidate;

        {
            scoped_lock lk(_lock);
            candidate = ReplicaSetMonitor::selectNode(_nodes, preference, tags,
                    _localThresholdMillis, &_lastReadPrefHost, isPrimarySelected);
        }

        if (candidate.empty()) {
            // mimic checkMaster behavior, which refreshes the local view of the replica set
            _check(false);

            scoped_lock lk(_lock);
            return ReplicaSetMonitor::selectNode(_nodes, preference, tags, _localThresholdMillis,
                    &_lastReadPrefHost, isPrimarySelected);
        }

        return candidate;
    }

    // static
    HostAndPort ReplicaSetMonitor::selectNode(const std::vector<Node>& nodes,
                                              ReadPreference preference,
                                              TagSet* tags,
                                              int localThresholdMillis,
                                              HostAndPort* lastHost,
                                              bool* isPrimarySelected) {
        *isPrimarySelected = false;

        switch (preference) {
        case ReadPreference_PrimaryOnly:
            for (vector<Node>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
                if (iter->ismaster && iter->ok) {
                    *isPrimarySelected = true;
                    return iter->addr;
                }
            }

            return HostAndPort();

        case ReadPreference_PrimaryPreferred:
        {
            HostAndPort candidatePri = selectNode(nodes, ReadPreference_PrimaryOnly, tags,
                    localThresholdMillis, lastHost, isPrimarySelected);

            if (!candidatePri.empty()) {
                return candidatePri;
            }

            return selectNode(nodes, ReadPreference_SecondaryOnly, tags,
                              localThresholdMillis, lastHost, isPrimarySelected);
        }

        case ReadPreference_SecondaryOnly:
        {
            HostAndPort candidate;

            while (!tags->isExhausted()) {
                candidate = _selectNode(nodes, tags->getCurrentTag(), true, localThresholdMillis,
                        lastHost, isPrimarySelected);

                if (candidate.empty()) {
                    tags->next();
                }
                else {
                    return candidate;
                }
            }

            return candidate;
        }

        case ReadPreference_SecondaryPreferred:
        {
            HostAndPort candidateSec = selectNode(nodes, ReadPreference_SecondaryOnly, tags,
                    localThresholdMillis, lastHost, isPrimarySelected);

            if (!candidateSec.empty()) {
                return candidateSec;
            }

            return selectNode(nodes, ReadPreference_PrimaryOnly, tags,
                    localThresholdMillis, lastHost, isPrimarySelected);
        }

        case ReadPreference_Nearest:
        {
            HostAndPort candidate;

            while (!tags->isExhausted()) {
                candidate = _selectNode(nodes, tags->getCurrentTag(), false, localThresholdMillis,
                        lastHost, isPrimarySelected);

                if (candidate.empty()) {
                    tags->next();
                }
                else {
                    return candidate;
                }
            }

            return candidate;
        }

        default:
            uassert( 16337, "Unknown read preference", false );
            break;
        }

        return HostAndPort();
    }

    bool ReplicaSetMonitor::isHostCompatible(const HostAndPort& host,
                                             ReadPreference readPreference,
                                             const TagSet* tagSet) const {
        scoped_lock lk(_lock);
        for (vector<Node>::const_iterator iter = _nodes.begin(); iter != _nodes.end(); ++iter) {
            if (iter->addr == host) {
                return iter->isCompatible(readPreference, tagSet);
            }
        }

        // host is not part of the set anymore!
        return false;
    }

    void ReplicaSetMonitor::_populateHosts_inSetsLock(const vector<HostAndPort>& seedList){
        verify(_nodes.empty());

        for (vector<HostAndPort>::const_iterator iter = seedList.begin();
                iter != seedList.end(); ++iter) {
            // Don't check servers we have already
            if (_find(*iter) >= 0) continue;

            ConnectionString connStr(*iter);
            scoped_ptr<DBClientConnection> conn;

            uassert(16531, str::stream() << "cannot create a replSet node connection that "
                    "is not single: " << iter->toString(true),
                    connStr.type() == ConnectionString::MASTER ||
                    connStr.type() == ConnectionString::CUSTOM);

            string errmsg;
            try {
                // Needs to perform a dynamic_cast because we need to set the replSet
                // callback. We should eventually not need this after we remove the
                // callback.
                conn.reset(dynamic_cast<DBClientConnection*>(
                        connStr.connect(errmsg, SOCKET_TIMEOUT_SECS)));
            }
            catch (const AssertionException& ex) {
                errmsg = ex.toString();
            }

            if (conn.get() != NULL && errmsg.empty()) {
                log() << "successfully connected to seed " << *iter
                        << " for replica set " << _name << endl;

                string maybePrimary;
                _checkConnection(conn.get(), maybePrimary, false, -1);
            }
            else {
                log() << "error connecting to seed " << *iter
                      << ", err: " << errmsg << endl;
            }
        }

        // Check everything to get the first data
        _check(true);
    }

    bool ReplicaSetMonitor::isAnyNodeOk() const {
        scoped_lock lock(_lock);

        for (vector<Node>::const_iterator iter = _nodes.begin();
                iter != _nodes.end(); ++iter) {
            if (iter->ok) {
                return true;
            }
        }

        return false;
    }

    bool ReplicaSetMonitor::Node::matchesTag(const BSONObj& tag) const {
        if (tag.isEmpty()) {
            return true;
        }

        const BSONElement& myTagElem = lastIsMaster["tags"];
        if (!myTagElem.isABSONObj()) {
            return false;
        }

        const BSONObj& myTagObj = myTagElem.Obj();
        for (BSONObjIterator iter(tag); iter.more();) {
            const BSONElement& tagCriteria(iter.next());
            const char* field = tagCriteria.fieldName();

            if (!myTagObj.hasField(field) ||
                    !tagCriteria.valuesEqual(myTagObj[field])) {
                return false;
            }
        }

        return true;
    }

    bool ReplicaSetMonitor::Node::isCompatible(ReadPreference readPreference,
                                               const TagSet* tags) const {
        if (!ok) {
            return false;
        }

        if ((readPreference == ReadPreference_SecondaryOnly ||
                /* This is the original behavior for slaveOk. This can result to reading
                 * data back in time, but the main idea here is to avoid overloading the
                 * primary when secondary is available.
                 */
                readPreference == ReadPreference_SecondaryPreferred) &&
                !okForSecondaryQueries()) {
            return false;
        }

        if ((readPreference == ReadPreference_PrimaryOnly ||
                readPreference == ReadPreference_PrimaryPreferred) &&
                secondary) {
            return false;
        }

        scoped_ptr<BSONObjIterator> bsonIter(tags->getIterator());
        if (!bsonIter->more()) {
            // Empty tag set
            return true;
        }

        while (bsonIter->more()) {
            const BSONElement& nextTag = bsonIter->next();
            uassert(16358, "Tags should be a BSON object", nextTag.isABSONObj());

            if (matchesTag(nextTag.Obj())) {
                return true;
            }
        }

        return false;
    }

    BSONObj ReplicaSetMonitor::Node::toBSON() const {
        BSONObjBuilder builder;
        builder.append( "addr", addr.toString() );
        builder.append( "isMaster", ismaster );
        builder.append( "secondary", secondary );
        builder.append( "hidden", hidden );

        const BSONElement& tagElem = lastIsMaster["tags"];
        if ( tagElem.ok() && tagElem.isABSONObj() ){
            builder.append( "tags", tagElem.Obj() );
        }

        builder.append( "ok", ok );

        return builder.obj();
    }

    mongo::mutex ReplicaSetMonitor::_setsLock( "ReplicaSetMonitor" );
    map<string,ReplicaSetMonitorPtr> ReplicaSetMonitor::_sets;
    map<string,vector<HostAndPort> > ReplicaSetMonitor::_seedServers;
    ReplicaSetMonitor::ConfigChangeHook ReplicaSetMonitor::_hook;
    int ReplicaSetMonitor::_maxFailedChecks = 30; // At 1 check every 10 seconds, 30 checks takes 5 minutes

    // --------------------------------
    // ----- DBClientReplicaSet ---------
    // --------------------------------

    const size_t DBClientReplicaSet::MAX_RETRY = 3;

    DBClientReplicaSet::DBClientReplicaSet( const string& name , const vector<HostAndPort>& servers, double so_timeout )
        : _setName( name ), _so_timeout( so_timeout ) {
        ReplicaSetMonitor::createIfNeeded( name, servers );
    }

    DBClientReplicaSet::~DBClientReplicaSet() {
    }

    ReplicaSetMonitorPtr DBClientReplicaSet::_getMonitor() const {
        ReplicaSetMonitorPtr rsm = ReplicaSetMonitor::get( _setName, true );
        // If you can't get a ReplicaSetMonitor then this connection isn't valid
        uassert( 16340, str::stream() << "No replica set monitor active and no cached seed "
                 "found for set: " << _setName, rsm );
        return rsm;
    }

    // This can't throw an exception because it is called in the destructor of ScopedDbConnection
    string DBClientReplicaSet::getServerAddress() const {
        ReplicaSetMonitorPtr rsm = ReplicaSetMonitor::get( _setName, true );
        if ( !rsm ) {
            warning() << "Trying to get server address for DBClientReplicaSet, but no "
                "ReplicaSetMonitor exists for " << _setName << endl;
            return str::stream() << _setName << "/" ;
        }
        return rsm->getServerAddress();
    }

    DBClientConnection * DBClientReplicaSet::checkMaster() {
        ReplicaSetMonitorPtr monitor = _getMonitor();
        HostAndPort h = monitor->getMaster();

        if ( h == _masterHost && _master ) {
            // a master is selected.  let's just make sure connection didn't die
            if ( ! _master->isFailed() )
                return _master.get();
            monitor->notifyFailure( _masterHost );
        }

        _masterHost = monitor->getMaster();

        ConnectionString connStr(_masterHost);

        string errmsg;
        DBClientConnection* newConn = NULL;

        try {
            // Needs to perform a dynamic_cast because we need to set the replSet
            // callback. We should eventually not need this after we remove the
            // callback.
            newConn = dynamic_cast<DBClientConnection*>(
                    connStr.connect(errmsg, _so_timeout));
        }
        catch (const AssertionException& ex) {
            errmsg = ex.toString();
        }

        if (newConn == NULL || !errmsg.empty()) {
            monitor->notifyFailure(_masterHost);
            uasserted(13639, str::stream() << "can't connect to new replica set master ["
                      << _masterHost.toString() << "]"
                      << (errmsg.empty()? "" : ", err: ") << errmsg);
        }

        _master.reset(newConn);
        _master->setReplSetClientCallback(this);

        _auth( _master.get() );
        return _master.get();
    }

    bool DBClientReplicaSet::checkLastHost(ReadPreference preference, const TagSet* tags) {
        if (_lastSlaveOkHost.empty()) {
            return false;
        }

        ReplicaSetMonitorPtr monitor = _getMonitor();

        if (_lastSlaveOkConn && _lastSlaveOkConn->isFailed()) {
            invalidateLastSlaveOkCache();
            return false;
        }

        return _lastSlaveOkConn && monitor->isHostCompatible(_lastSlaveOkHost, preference, tags);
    }

    void DBClientReplicaSet::_auth( DBClientConnection * conn ) {
        for (map<string, AuthInfo>::const_iterator i = _auths.begin(); i != _auths.end(); ++i) {
            const AuthInfo& a = i->second;
            string errmsg;
            if ( ! conn->auth( a.dbname , a.username , a.pwd , errmsg, a.digestPassword ) )
                warning() << "cached auth failed for set: " << _setName << " db: " << a.dbname << " user: " << a.username << endl;

        }
    }

    DBClientConnection& DBClientReplicaSet::masterConn() {
        return *checkMaster();
    }

    DBClientConnection& DBClientReplicaSet::slaveConn() {
        BSONArray emptyArray;
        TagSet tags( emptyArray );
        DBClientConnection* conn = selectNodeUsingTags( ReadPreference_SecondaryPreferred,
                &tags );

        uassert( 16369, str::stream() << "No good nodes available for set: "
                 << _getMonitor()->getName(), conn != NULL );

        return *conn;
    }

    bool DBClientReplicaSet::connect() {
        return _getMonitor()->isAnyNodeOk();
    }

    bool DBClientReplicaSet::auth(const string &dbname, const string &username, const string &pwd, string& errmsg, bool digestPassword, Auth::Level * level) {
        DBClientConnection * m = checkMaster();

        // first make sure it actually works
        if( ! m->auth(dbname, username, pwd, errmsg, digestPassword, level ) )
            return false;

        /* Also authenticate the cached secondary connection. Note that this is only
         * needed when we actually have something cached and is last known to be
         * working.
         */
        if (_lastSlaveOkConn.get() != NULL && !_lastSlaveOkConn->isFailed()) {
            try {
                _lastSlaveOkConn->auth(dbname, username, pwd, errmsg, digestPassword, level);
            }
            catch (const DBException&) {
                /* Swallow exception. _lastSlaveOkConn is now in failed state.
                 * The next time we create a new secondary connection it will
                 * be authenticated with the credentials from _auths.
                 */
                verify(_lastSlaveOkConn->isFailed());
            }
        }

        // now that it does, we should save so that for a new node we can auth
        _auths[dbname] = AuthInfo(dbname, username, pwd, digestPassword);
        return true;
    }

    void DBClientReplicaSet::logout(const string &dbname, BSONObj& info) {
        DBClientConnection* priConn = checkMaster();

        priConn->logout(dbname, info);
        _auths.erase(dbname);

        /* Also logout the cached secondary connection. Note that this is only
         * needed when we actually have something cached and is last known to be
         * working.
         */
        if (_lastSlaveOkConn.get() != NULL && !_lastSlaveOkConn->isFailed()) {
            try {
                BSONObj dummy;
                _lastSlaveOkConn->logout(dbname, dummy);
            }
            catch (const DBException&) {
                // Make sure we can't use this connection again.
                verify(_lastSlaveOkConn->isFailed());
            }
        }
    }

    // ------------- simple functions -----------------

    void DBClientReplicaSet::insert( const string &ns , BSONObj obj , int flags) {
        checkMaster()->insert(ns, obj, flags);
    }

    void DBClientReplicaSet::insert( const string &ns, const vector< BSONObj >& v , int flags) {
        checkMaster()->insert(ns, v, flags);
    }

    void DBClientReplicaSet::remove( const string &ns , Query obj , int flags ) {
        checkMaster()->remove(ns, obj, flags);
    }

    void DBClientReplicaSet::update( const string &ns , Query query , BSONObj obj , int flags ) {
        return checkMaster()->update( ns, query, obj, flags );
    }

    auto_ptr<DBClientCursor> DBClientReplicaSet::query(const string &ns,
                                                       Query query,
                                                       int nToReturn,
                                                       int nToSkip,
                                                       const BSONObj *fieldsToReturn,
                                                       int queryOptions,
                                                       int batchSize) {
        if (_isQueryOkToSecondary(ns, queryOptions, query.obj)) {
            ReadPreference pref;
            scoped_ptr<TagSet> tags(_extractReadPref(query.obj, &pref));

            for (size_t retry = 0; retry < MAX_RETRY; retry++) {
                try {
                    DBClientConnection* conn = selectNodeUsingTags(pref, tags.get());

                    if (conn == NULL) {
                        break;
                    }

                    auto_ptr<DBClientCursor> cursor = conn->query(ns, query,
                            nToReturn, nToSkip, fieldsToReturn, queryOptions,
                            batchSize);

                    return checkSlaveQueryResult(cursor);
                }
                catch (const DBException &dbExcep) {
                    LOG(1) << "can't query replica set slave " << _lastSlaveOkHost
                           << ": " << causedBy(dbExcep) << endl;
                    invalidateLastSlaveOkCache();
                }
            }

            uasserted(16370, str::stream() << "Failed to do query, no good nodes in "
                        << _getMonitor()->getName());
        }

        return checkMaster()->query(ns, query, nToReturn, nToSkip, fieldsToReturn,
                queryOptions, batchSize);
    }

    BSONObj DBClientReplicaSet::findOne(const string &ns,
                                        const Query& query,
                                        const BSONObj *fieldsToReturn,
                                        int queryOptions) {
        if (_isQueryOkToSecondary(ns, queryOptions, query.obj)) {
            ReadPreference pref;
            scoped_ptr<TagSet> tags(_extractReadPref(query.obj, &pref));

            for (size_t retry = 0; retry < MAX_RETRY; retry++) {
                try {
                    DBClientConnection* conn = selectNodeUsingTags(pref, tags.get());

                    if (conn == NULL) {
                        break;
                    }

                    return conn->findOne(ns,query,fieldsToReturn,queryOptions);
                }
                catch (const DBException &dbExcep) {
                    LOG(1) << "can't findone replica set slave " << _lastSlaveOkHost
                           << ": " << causedBy(dbExcep) << endl;
                    invalidateLastSlaveOkCache();
                }
            }

            uasserted(16379, str::stream() << "Failed to call findOne, no good nodes in "
                        << _getMonitor()->getName());
        }

        return checkMaster()->findOne(ns,query,fieldsToReturn,queryOptions);
    }

    void DBClientReplicaSet::killCursor( long long cursorID ) {
        // we should never call killCursor on a replica set connection
        // since we don't know which server it belongs to
        // can't assume master because of slave ok
        // and can have a cursor survive a master change
        verify(0);
    }

    void DBClientReplicaSet::isntMaster() { 
        log() << "got not master for: " << _masterHost << endl;
        // Can't use _getMonitor because that will create a new monitor from the cached seed if
        // the monitor doesn't exist.
        ReplicaSetMonitorPtr monitor = ReplicaSetMonitor::get( _setName );
        if ( monitor ) {
            monitor->notifyFailure( _masterHost );
        }
        _master.reset(); 
    }

    auto_ptr<DBClientCursor> DBClientReplicaSet::checkSlaveQueryResult( auto_ptr<DBClientCursor> result ){
        if ( result.get() == NULL ) return result;

        BSONObj error;
        bool isError = result->peekError( &error );
        if( ! isError ) return result;

        // We only check for "not master or secondary" errors here

        // If the error code here ever changes, we need to change this code also
        BSONElement code = error["code"];
        if( code.isNumber() && code.Int() == 13436 /* not master or secondary */ ){
            isntSecondary();
            throw DBException( str::stream() << "slave " << _lastSlaveOkHost.toString()
                    << " is no longer secondary", 14812 );
        }

        return result;
    }

    void DBClientReplicaSet::isntSecondary() {
        log() << "slave no longer has secondary status: " << _lastSlaveOkHost << endl;
        // Failover to next slave
        _getMonitor()->notifySlaveFailure( _lastSlaveOkHost );
        _lastSlaveOkConn.reset();
    }

    DBClientConnection* DBClientReplicaSet::selectNodeUsingTags(ReadPreference preference,
                                                                TagSet* tags) {
        if (checkLastHost(preference, tags)) {
            return _lastSlaveOkConn.get();
        }

        ReplicaSetMonitorPtr monitor = _getMonitor();
        bool isPrimarySelected = false;
        _lastSlaveOkHost = monitor->selectAndCheckNode(preference, tags, &isPrimarySelected);

        if ( _lastSlaveOkHost.empty() ){
            return NULL;
        }

        // Primary connection is special because it is the only connection that is
        // versioned in mongos. Therefore, we have to make sure that this object
        // maintains only one connection to the primary and use that connection
        // every time we need to talk to the primary.
        if (isPrimarySelected) {
            checkMaster();
            _lastSlaveOkConn = _master;
            _lastSlaveOkHost = _masterHost; // implied, but still assign just to be safe
            return _master.get();
        }

        string errmsg;
        ConnectionString connStr(_lastSlaveOkHost);
        // Needs to perform a dynamic_cast because we need to set the replSet
        // callback. We should eventually not need this after we remove the
        // callback.
        DBClientConnection* newConn = dynamic_cast<DBClientConnection*>(
                connStr.connect(errmsg, _so_timeout));

        // Assert here instead of returning NULL since the contract of this method is such
        // that returning NULL means none of the nodes were good, which is not the case here.
        uassert(16532, str::stream() << "Failed to connect to "
                << _lastSlaveOkHost.toString(true),
                newConn != NULL);

        _lastSlaveOkConn.reset(newConn);
        _lastSlaveOkConn->setReplSetClientCallback(this);

        _auth(_lastSlaveOkConn.get());
        return _lastSlaveOkConn.get();
    }

    void DBClientReplicaSet::say(Message& toSend, bool isRetry, string* actualServer) {

        if (!isRetry)
            _lazyState = LazyState();

        const int lastOp = toSend.operation();
        bool slaveOk = false;

        if (lastOp == dbQuery) {
            // TODO: might be possible to do this faster by changing api
            DbMessage dm(toSend);
            QueryMessage qm(dm);

            const bool slaveOk = qm.queryOptions & QueryOption_SlaveOk;
            if (_isQueryOkToSecondary(qm.ns, qm.queryOptions, qm.query)) {
                ReadPreference pref;
                scoped_ptr<TagSet> tags(_extractReadPref(qm.query, &pref));

                for (size_t retry = 0; retry < MAX_RETRY; retry++) {
                    _lazyState._retries = retry;
                    try {
                        DBClientConnection* conn = selectNodeUsingTags(pref, tags.get());

                        if (conn == NULL) {
                            break;
                        }

                        if (actualServer != NULL) {
                            *actualServer = conn->getServerAddress();
                        }

                        conn->say(toSend);

                        _lazyState._lastOp = lastOp;
                        _lazyState._slaveOk = slaveOk;
                        _lazyState._lastClient = conn;
                    }
                    catch (const DBException& DBExcep) {
                        LOG(1) << "can't callLazy replica set slave " << _lastSlaveOkHost
                                << ": " << causedBy(DBExcep) << endl;
                        invalidateLastSlaveOkCache();
                        continue;
                    }

                    return;
                }

                uasserted(16380, str::stream() << "Failed to call say, no good nodes in "
                         << _getMonitor()->getName());
            }
        }

        DBClientConnection* master = checkMaster();
        if (actualServer)
            *actualServer = master->getServerAddress();

        _lazyState._lastOp = lastOp;
        _lazyState._slaveOk = slaveOk;
        // Don't retry requests to primary since there is only one host to try
        _lazyState._retries = MAX_RETRY;
        _lazyState._lastClient = master;

        master->say(toSend);
        return;
    }

    bool DBClientReplicaSet::recv( Message& m ) {

        verify( _lazyState._lastClient );

        // TODO: It would be nice if we could easily wrap a conn error as a result error
        try {
            return _lazyState._lastClient->recv( m );
        }
        catch( DBException& e ){
            log() << "could not receive data from " << _lazyState._lastClient << causedBy( e ) << endl;
            return false;
        }
    }

    void DBClientReplicaSet::checkResponse( const char* data, int nReturned, bool* retry, string* targetHost ){

        // For now, do exactly as we did before, so as not to break things.  In general though, we
        // should fix this so checkResponse has a more consistent contract.
        if( ! retry ){
            if( _lazyState._lastClient )
                return _lazyState._lastClient->checkResponse( data, nReturned );
            else
                return checkMaster()->checkResponse( data, nReturned );
        }

        *retry = false;
        if( targetHost && _lazyState._lastClient ) *targetHost = _lazyState._lastClient->getServerAddress();
        else if (targetHost) *targetHost = "";

        if( ! _lazyState._lastClient ) return;
        if( nReturned != 1 && nReturned != -1 ) return;

        BSONObj dataObj;
        if( nReturned == 1 ) dataObj = BSONObj( data );

        // Check if we should retry here
        if( _lazyState._lastOp == dbQuery && _lazyState._slaveOk ){

            // Check the error code for a slave not secondary error
            if( nReturned == -1 ||
                ( hasErrField( dataObj ) &&  ! dataObj["code"].eoo() && dataObj["code"].Int() == 13436 ) ){

                bool wasMaster = false;
                if( _lazyState._lastClient == _lastSlaveOkConn.get() ){
                    isntSecondary();
                }
                else if( _lazyState._lastClient == _master.get() ){
                    wasMaster = true;
                    isntMaster();
                }
                else
                    warning() << "passed " << dataObj << " but last rs client " << _lazyState._lastClient->toString() << " is not master or secondary" << endl;

                if( _lazyState._retries < 3 ){
                    _lazyState._retries++;
                    *retry = true;
                }
                else{
                    (void)wasMaster; // silence set-but-not-used warning
                    // verify( wasMaster );
                    // printStackTrace();
                    log() << "too many retries (" << _lazyState._retries << "), could not get data from replica set" << endl;
                }
            }
        }
        else if( _lazyState._lastOp == dbQuery ){
            // slaveOk is not set, just mark the master as bad

            if( nReturned == -1 ||
               ( hasErrField( dataObj ) &&  ! dataObj["code"].eoo() && dataObj["code"].Int() == 13435 ) )
            {
                if( _lazyState._lastClient == _master.get() ){
                    isntMaster();
                }
            }
        }
    }


    bool DBClientReplicaSet::call(Message &toSend,
                                  Message &response,
                                  bool assertOk,
                                  string * actualServer) {
        const char * ns = 0;

        if (toSend.operation() == dbQuery) {
            // TODO: might be possible to do this faster by changing api
            DbMessage dm(toSend);
            QueryMessage qm(dm);
            ns = qm.ns;

            if (_isQueryOkToSecondary(ns, qm.queryOptions, qm.query)) {
                ReadPreference pref;
                scoped_ptr<TagSet> tags(_extractReadPref(qm.query, &pref));

                for (size_t retry = 0; retry < MAX_RETRY; retry++) {
                    try {
                        DBClientConnection* conn = selectNodeUsingTags(pref, tags.get());

                        if (conn == NULL) {
                            return false;
                        }

                        if (actualServer != NULL) {
                            *actualServer = conn->getServerAddress();
                        }

                        return conn->call(toSend, response, assertOk);
                    }
                    catch (const DBException& dbExcep) {
                        LOG(1) << "can't call replica set slave " << _lastSlaveOkHost
                               << ": " << causedBy(dbExcep) << endl;

                        if (actualServer)
                            *actualServer = "";

                        invalidateLastSlaveOkCache();
                    }
                }

                // Was not able to successfully send after max retries
                return false;
            }
        }
        
        DBClientConnection* m = checkMaster();
        if ( actualServer )
            *actualServer = m->getServerAddress();
        
        if ( ! m->call( toSend , response , assertOk ) )
            return false;

        if ( ns ) {
            QueryResult * res = (QueryResult*)response.singleData();
            if ( res->nReturned == 1 ) {
                BSONObj x(res->data() );
                if ( str::contains( ns , "$cmd" ) ) {
                    if ( isNotMasterErrorString( x["errmsg"] ) )
                        isntMaster();
                }
                else {
                    if ( isNotMasterErrorString( getErrField( x ) ) )
                        isntMaster();
                }
            }
        }

        return true;
    }

    void DBClientReplicaSet::invalidateLastSlaveOkCache() {
        /* This is not wrapped in with if (_lastSlaveOkConn && _lastSlaveOkConn->isFailed())
         * because there are certain exceptions that will not make the connection be labeled
         * as failed. For example, asserts 13079, 13080, 16386
         */
        _getMonitor()->notifySlaveFailure(_lastSlaveOkHost);
        _lastSlaveOkHost = HostAndPort();
        _lastSlaveOkConn.reset();
    }

    TagSet::TagSet() : _isExhausted(true), _tagIterator(_tags) {
    }

    TagSet::TagSet(const BSONArray& tags) :
            _isExhausted(false),
            _tags(tags.getOwned()),
            _tagIterator(_tags) {
        next();
    }

    void TagSet::next() {
        if (_tagIterator.more()) {
            const BSONElement& nextTag = _tagIterator.next();
            uassert(16357, "Tags should be a BSON object", nextTag.isABSONObj());
            _currentTag = nextTag.Obj();
        }
        else {
            _isExhausted = true;
        }
    }

    const BSONObj& TagSet::getCurrentTag() const {
        verify(!_isExhausted);
        return _currentTag;
    }

    bool TagSet::isExhausted() const {
        return _isExhausted;
    }

    BSONObjIterator* TagSet::getIterator() const {
        return new BSONObjIterator(_tags);
    }
}
