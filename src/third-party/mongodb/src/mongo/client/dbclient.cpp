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

#include "mongo/bson/util/builder.h"
#include "mongo/client/constants.h"
#include "mongo/client/dbclient_rs.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/client/syncclusterconnection.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/json.h"
#include "mongo/db/namespace-inl.h"
#include "mongo/db/namespacestring.h"
#include "mongo/s/stale_exception.h"  // for RecvStaleConfigException
#include "mongo/util/md5.hpp"

#ifdef MONGO_SSL
// TODO: Remove references to cmdline from the client.
#include "mongo/db/cmdline.h"
#endif  // defined MONGO_SSL

#ifdef ROBOMONGO
#include "libssh2.h"
namespace
{
    static void kbd_callback(const char *name, int name_len,
        const char *instruction, int instruction_len,
        int num_prompts,
        const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
        LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
        void **abstract)
    {

    } /* kbd_callback */ 
}
namespace Robomongo
{
    class SSHSocket :public mongo::Socket
    {
    public: 
        typedef mongo::Socket BaseClass;
        SSHSocket(const SSHInfo &info, int sock, const mongo::SockAddr& farEnd): 
            BaseClass(sock, farEnd),_info(info),_session(NULL),_channel(NULL){}
        SSHSocket(const SSHInfo &info, double so_timeout = 0, int logLevel = 0 ): 
            BaseClass(so_timeout, logLevel),_info(info),_session(NULL),_channel(NULL){}

        virtual bool connect(mongo::SockAddr& farEnd)
        {            
            static int rc = libssh2_init (0);
            if (rc != 0) {
                return false;
            }
            const char *username = _info._userName.c_str();
            const char *password = _info._password.c_str();
            const char *address = _info._hostName.c_str();
            const char *publicKey = _info._publicKey._publicKey.empty() ? NULL : _info._publicKey._publicKey.c_str();
            const char *privateKey = _info._publicKey._privateKey.c_str();
            const char *passphrase = _info._publicKey._passphrase.c_str();
            int port = _info._port; 

            mongo::SockAddr sshAdr(address,port);

            bool connected = BaseClass::connect(sshAdr);
            if(!connected)
                return false;

            _session = libssh2_session_init();
            if (libssh2_session_handshake(_session, _fd)) {
                return false;
            }

            int auth_pw = 0;
            const char *fingerprint = libssh2_hostkey_hash(_session, LIBSSH2_HOSTKEY_HASH_SHA1);
            char *userauthlist = libssh2_userauth_list(_session, username, strlen(username));
            if (strstr(userauthlist, "password") != NULL) {
                auth_pw |= 1;
            }
            if (strstr(userauthlist, "keyboard-interactive") != NULL) {
                auth_pw |= 2;
            }
            if (strstr(userauthlist, "publickey") != NULL) {
                auth_pw |= 4;
            }
            SSHInfo::SupportedAuthenticationMetods curMethod = _info.authMethod();
            if (auth_pw & 1 && curMethod == SSHInfo::PASSWORD) {
                /* We could authenticate via password */ 
                if (libssh2_userauth_password(_session, username, password)) {
                    LOG(mongo::LL_ERROR) << "Authentication by password failed!";
                    return false;
                }
            } else if (auth_pw & 2) {
                /* Or via keyboard-interactive */ 
                if (libssh2_userauth_keyboard_interactive(_session, username, &kbd_callback) ) 
                {
                    LOG(mongo::LL_ERROR) << "Authentication by keyboard-interactive failed!";
                    return false;
                }
            } else if (auth_pw & 4 && curMethod == SSHInfo::PUBLICKEY) {
                /* Or by public key */ 
                if (libssh2_userauth_publickey_fromfile(_session, username, publicKey, privateKey, passphrase)) 
                {
                    LOG(mongo::LL_ERROR) << "Authentication by public key failed!";
                    return false;
                }
            } else {
                LOG(mongo::LL_ERROR) << "No supported authentication methods found!";
                return false;
            }
            /* Request a shell */ 
            if (!(_channel = libssh2_channel_direct_tcpip(_session,farEnd.getAddr().c_str(),farEnd.getPort()))) {
                LOG(mongo::LL_ERROR) << "Unable to open a ssh session";
                return false;
            }
            
            return true;
        }
    private:
        virtual int _send( const char * data , int len )
        {
#ifdef MONGO_SSL
            if ( _ssl ) {
                return libssh2_channel_write_ssl(_channel, _ssl, data, len );
            }
#endif
            return libssh2_channel_write(_channel,data,len);
        }

        virtual int _recv( char * buf , int max )
        {
#ifdef MONGO_SSL
            if ( _ssl ){
                return libssh2_channel_read_ssl(_channel, _ssl, buf, max );
            }
#endif
            return libssh2_channel_read(_channel,buf,max);
        }

        const SSHInfo _info;
        LIBSSH2_SESSION *_session;
        LIBSSH2_CHANNEL *_channel;
    };
}
#endif

namespace mongo {

    AtomicInt64 DBClientBase::ConnectionIdSequence;

    bool hasReadPreference(const BSONObj& queryObj) {
        const bool isQueryEmbedded = strcmp(queryObj.firstElement().fieldName(), "query") == 0;
        const bool hasReadPrefOption = queryObj["$queryOptions"].isABSONObj() &&
                        queryObj["$queryOptions"].Obj().hasField("$readPreference");
        return (isQueryEmbedded && queryObj.hasField("$readPreference")) || hasReadPrefOption;
    }

    void ConnectionString::_fillServers( string s ) {
        
        //
        // Custom-handled servers/replica sets start with '$'
        // According to RFC-1123/952, this will not overlap with valid hostnames
        // (also disallows $replicaSetName hosts)
        //

        if( s.find( '$' ) == 0 ) _type = CUSTOM;

        {
            string::size_type idx = s.find( '/' );
            if ( idx != string::npos ) {
                _setName = s.substr( 0 , idx );
                s = s.substr( idx + 1 );
                if( _type != CUSTOM ) _type = SET;
            }
        }

        string::size_type idx;
        while ( ( idx = s.find( ',' ) ) != string::npos ) {
            _servers.push_back( s.substr( 0 , idx ) );
            s = s.substr( idx + 1 );
        }
        _servers.push_back( s );

    }
    
    void ConnectionString::_finishInit() {

        // Needed here as well b/c the parsing logic isn't used in all constructors
        // TODO: Refactor so that the parsing logic *is* used in all constructors
        if ( _type == MASTER && _servers.size() > 0 ){
            if( _servers[0].host().find( '$' ) == 0 ){
                _type = CUSTOM;
            }
        }

        stringstream ss;
        if ( _type == SET )
            ss << _setName << "/";
        for ( unsigned i=0; i<_servers.size(); i++ ) {
            if ( i > 0 )
                ss << ",";
            ss << _servers[i].toString();
        }
        _string = ss.str();
    }

    mutex ConnectionString::_connectHookMutex( "ConnectionString::_connectHook" );
    ConnectionString::ConnectionHook* ConnectionString::_connectHook = NULL;

    DBClientBase* ConnectionString::connect( string& errmsg, double socketTimeout ) const {

        switch ( _type ) {
        case MASTER: {
            DBClientConnection * c = new DBClientConnection(true);
            c->setSoTimeout( socketTimeout );
            LOG(1) << "creating new connection to:" << _servers[0] << endl;
            if ( ! c->connect( _servers[0] , errmsg ) ) {
                delete c;
                return 0;
            }
            LOG(1) << "connected connection!" << endl;
            return c;
        }

        case PAIR:
        case SET: {
            DBClientReplicaSet * set = new DBClientReplicaSet( _setName , _servers , socketTimeout );
            if( ! set->connect() ) {
                delete set;
                errmsg = "connect failed to replica set ";
                errmsg += toString();
                return 0;
            }
            return set;
        }

        case SYNC: {
            // TODO , don't copy
            list<HostAndPort> l;
            for ( unsigned i=0; i<_servers.size(); i++ )
                l.push_back( _servers[i] );
            SyncClusterConnection* c = new SyncClusterConnection( l, socketTimeout );
            return c;
        }

        case CUSTOM: {

            // Lock in case other things are modifying this at the same time
            scoped_lock lk( _connectHookMutex );

            // Allow the replacement of connections with other connections - useful for testing.

            uassert( 16335, "custom connection to " + this->toString() +
                        " specified with no connection hook", _connectHook );

            // Double-checked lock, since this will never be active during normal operation
            DBClientBase* replacementConn = _connectHook->connect( *this, errmsg, socketTimeout );

            log() << "replacing connection to " << this->toString() << " with "
                  << ( replacementConn ? replacementConn->getServerAddress() : "(empty)" ) << endl;

            return replacementConn;
        }

        case INVALID:
            throw UserException( 13421 , "trying to connect to invalid ConnectionString" );
            break;
        }

        verify( 0 );
        return 0;
    }

    bool ConnectionString::sameLogicalEndpoint( const ConnectionString& other ) const {
        if ( _type != other._type )
            return false;

        switch ( _type ) {
        case INVALID:
            return true;
        case MASTER:
            return _servers[0] == other._servers[0];
        case PAIR:
            if ( _servers[0] == other._servers[0] )
                return _servers[1] == other._servers[1];
            return
                ( _servers[0] == other._servers[1] ) &&
                ( _servers[1] == other._servers[0] );
        case SET:
            return _setName == other._setName;
        case SYNC:
            // The servers all have to be the same in each, but not in the same order.
            if ( _servers.size() != other._servers.size() )
                return false;
            for ( unsigned i = 0; i < _servers.size(); i++ ) {
                bool found = false;
                for ( unsigned j = 0; j < other._servers.size(); j++ ) {
                    if ( _servers[i] == other._servers[j] ) {
                        found = true;
                        break;
                    }
                }
                if ( ! found )
                    return false;
            }
            return true;
        case CUSTOM:
            return _string == other._string;
        }
        verify( false );
    }

    ConnectionString ConnectionString::parse( const string& host , string& errmsg ) {

        string::size_type i = host.find( '/' );
#ifdef ROBOMONGO
        string hostWithoutOptions = host;
        string::size_type s = host.find_first_of( '{' );
        string::size_type j = host.find( ':' ); 
        if(s!= string::npos){
            hostWithoutOptions = host.substr(0,s);
        }
        if ( i != string::npos  && i != 0 && i < j ) {
            // replica set
            return ConnectionString( SET , host.substr( i + 1 ) , host.substr( 0 , i ) );
        }
#else
        if ( i != string::npos && i != 0 ) {
            // replica set
            return ConnectionString( SET , host.substr( i + 1 ) , host.substr( 0 , i ) );
        }
#endif

#ifdef ROBOMONGO
        int numCommas = str::count( hostWithoutOptions , ',' );
#else
        int numCommas = str::count( host , ',' );
#endif
        if( numCommas == 0 )
            return ConnectionString( HostAndPort( host ) );

        if ( numCommas == 1 )
            return ConnectionString( PAIR , host );

        if ( numCommas == 2 )
            return ConnectionString( SYNC , host );

        errmsg = (string)"invalid hostname [" + host + "]";
        return ConnectionString(); // INVALID
    }

    string ConnectionString::typeToString( ConnectionType type ) {
        switch ( type ) {
        case INVALID:
            return "invalid";
        case MASTER:
            return "master";
        case PAIR:
            return "pair";
        case SET:
            return "set";
        case SYNC:
            return "sync";
        case CUSTOM:
            return "custom";
        }
        verify(0);
        return "";
    }


    Query::Query( const string &json ) : obj( fromjson( json ) ) {}

    Query::Query( const char *json ) : obj( fromjson( json ) ) {}

    Query& Query::hint(const string &jsonKeyPatt) { return hint( fromjson( jsonKeyPatt ) ); }

    Query& Query::where(const string &jscode, BSONObj scope) {
        /* use where() before sort() and hint() and explain(), else this will assert. */
        verify( ! isComplex() );
        BSONObjBuilder b;
        b.appendElements(obj);
        b.appendWhere(jscode, scope);
        obj = b.obj();
        return *this;
    }

    void Query::makeComplex() {
        if ( isComplex() )
            return;
        BSONObjBuilder b;
        b.append( "query", obj );
        obj = b.obj();
    }

    Query& Query::sort(const BSONObj& s) {
        appendComplex( "orderby", s );
        return *this;
    }

    Query& Query::hint(BSONObj keyPattern) {
        appendComplex( "$hint", keyPattern );
        return *this;
    }

    Query& Query::explain() {
        appendComplex( "$explain", true );
        return *this;
    }

    Query& Query::snapshot() {
        appendComplex( "$snapshot", true );
        return *this;
    }

    Query& Query::minKey( const BSONObj &val ) {
        appendComplex( "$min", val );
        return *this;
    }

    Query& Query::maxKey( const BSONObj &val ) {
        appendComplex( "$max", val );
        return *this;
    }

    bool Query::isComplex( bool * hasDollar ) const {
        if ( obj.hasElement( "query" ) ) {
            if ( hasDollar )
                hasDollar[0] = false;
            return true;
        }

        if ( obj.hasElement( "$query" ) ) {
            if ( hasDollar )
                hasDollar[0] = true;
            return true;
        }

        return false;
    }

    BSONObj Query::getFilter() const {
        bool hasDollar;
        if ( ! isComplex( &hasDollar ) )
            return obj;

        return obj.getObjectField( hasDollar ? "$query" : "query" );
    }
    BSONObj Query::getSort() const {
        if ( ! isComplex() )
            return BSONObj();
        BSONObj ret = obj.getObjectField( "orderby" );
        if (ret.isEmpty())
            ret = obj.getObjectField( "$orderby" );
        return ret;
    }
    BSONObj Query::getHint() const {
        if ( ! isComplex() )
            return BSONObj();
        return obj.getObjectField( "$hint" );
    }
    bool Query::isExplain() const {
        return isComplex() && obj.getBoolField( "$explain" );
    }

    string Query::toString() const {
        return obj.toString();
    }

    /* --- dbclientcommands --- */

    bool DBClientWithCommands::isOk(const BSONObj& o) {
        return o["ok"].trueValue();
    }

    bool DBClientWithCommands::isNotMasterErrorString( const BSONElement& e ) {
        return e.type() == String && str::contains( e.valuestr() , "not master" );
    }


    enum QueryOptions DBClientWithCommands::availableOptions() {
        if ( !_haveCachedAvailableOptions ) {
            _cachedAvailableOptions = _lookupAvailableOptions();
            _haveCachedAvailableOptions = true;
        }
        return _cachedAvailableOptions;
    }

    enum QueryOptions DBClientWithCommands::_lookupAvailableOptions() {
        BSONObj ret;
        if ( runCommand( "admin", BSON( "availablequeryoptions" << 1 ), ret ) ) {
            return QueryOptions( ret.getIntField( "options" ) );
        }
        return QueryOptions(0);
    }

    inline bool DBClientWithCommands::runCommand(const string &dbname,
                                                 const BSONObj& cmd,
                                                 BSONObj &info,
                                                 int options) {
        string ns = dbname + ".$cmd";
        info = findOne(ns, cmd, 0 , options);
        return isOk(info);
    }

    /* note - we build a bson obj here -- for something that is super common like getlasterror you
              should have that object prebuilt as that would be faster.
    */
    bool DBClientWithCommands::simpleCommand(const string &dbname, BSONObj *info, const string &command) {
        BSONObj o;
        if ( info == 0 )
            info = &o;
        BSONObjBuilder b;
        b.append(command, 1);
        return runCommand(dbname, b.done(), *info);
    }

    unsigned long long DBClientWithCommands::count(const string &myns, const BSONObj& query, int options, int limit, int skip ) {
        NamespaceString ns(myns);
        BSONObj cmd = _countCmd( myns , query , options , limit , skip );
        BSONObj res;
        if( !runCommand(ns.db.c_str(), cmd, res, options) )
            uasserted(11010,string("count fails:") + res.toString());
        return res["n"].numberLong();
    }

    BSONObj DBClientWithCommands::_countCmd(const string &myns, const BSONObj& query, int options, int limit, int skip ) {
        NamespaceString ns(myns);
        BSONObjBuilder b;
        b.append( "count" , ns.coll );
        b.append( "query" , query );
        if ( limit )
            b.append( "limit" , limit );
        if ( skip )
            b.append( "skip" , skip );
        return b.obj();
    }

    BSONObj DBClientWithCommands::getLastErrorDetailed(bool fsync, bool j, int w, int wtimeout) {
        return getLastErrorDetailed("admin", fsync, j, w, wtimeout);
    }

    BSONObj DBClientWithCommands::getLastErrorDetailed(const std::string& db,
                                                       bool fsync,
                                                       bool j,
                                                       int w,
                                                       int wtimeout) {
        BSONObj info;
        BSONObjBuilder b;
        b.append( "getlasterror", 1 );

        if ( fsync )
            b.append( "fsync", 1 );
        if ( j )
            b.append( "j", 1 );

        // only affects request when greater than one node
        if ( w >= 1 )
            b.append( "w", w );
        else if ( w == -1 )
            b.append( "w", "majority" );

        if ( wtimeout > 0 )
            b.append( "wtimeout", wtimeout );

        runCommand(db, b.obj(), info);

        return info;
    }

    string DBClientWithCommands::getLastError(bool fsync, bool j, int w, int wtimeout) {
        return getLastError("admin", fsync, j, w, wtimeout);
    }

    string DBClientWithCommands::getLastError(const std::string& db,
                                              bool fsync,
                                              bool j,
                                              int w,
                                              int wtimeout) {
        BSONObj info = getLastErrorDetailed(db, fsync, j, w, wtimeout);
        return getLastErrorString( info );
    }

    string DBClientWithCommands::getLastErrorString( const BSONObj& info ) {
        BSONElement e = info["err"];
        if( e.eoo() ) return "";
        if( e.type() == Object ) return e.toString();
        return e.str();
    }

    const BSONObj getpreverrorcmdobj = fromjson("{getpreverror:1}");

    BSONObj DBClientWithCommands::getPrevError() {
        BSONObj info;
        runCommand("admin", getpreverrorcmdobj, info);
        return info;
    }

    BSONObj getnoncecmdobj = fromjson("{getnonce:1}");

    string DBClientWithCommands::createPasswordDigest( const string & username , const string & clearTextPassword ) {
        md5digest d;
        {
            md5_state_t st;
            md5_init(&st);
            md5_append(&st, (const md5_byte_t *) username.data(), username.length());
            md5_append(&st, (const md5_byte_t *) ":mongo:", 7 );
            md5_append(&st, (const md5_byte_t *) clearTextPassword.data(), clearTextPassword.length());
            md5_finish(&st, d);
        }
        return digestToString( d );
    }

    bool DBClientWithCommands::auth(const string &dbname, const string &username, const string &password_text, string& errmsg, bool digestPassword, Auth::Level * level) {
        string password = password_text;
        if( digestPassword )
            password = createPasswordDigest( username , password_text );

        if ( level != NULL )
                *level = Auth::NONE;

        BSONObj info;
        string nonce;
        if( !runCommand(dbname, getnoncecmdobj, info) ) {
            errmsg = "getnonce fails - connection problem?";
            return false;
        }
        {
            BSONElement e = info.getField("nonce");
            verify( e.type() == String );
            nonce = e.valuestr();
        }

        BSONObj authCmd;
        BSONObjBuilder b;
        {

            b << "authenticate" << 1 << "nonce" << nonce << "user" << username;
            md5digest d;
            {
                md5_state_t st;
                md5_init(&st);
                md5_append(&st, (const md5_byte_t *) nonce.c_str(), nonce.size() );
                md5_append(&st, (const md5_byte_t *) username.data(), username.length());
                md5_append(&st, (const md5_byte_t *) password.c_str(), password.size() );
                md5_finish(&st, d);
            }
            b << "key" << digestToString( d );
            authCmd = b.done();
        }

        if( runCommand(dbname, authCmd, info) ) {
            if ( level != NULL ) {
                if ( info.getField("readOnly").trueValue() )
                    *level = Auth::READ;
                else
                    *level = Auth::WRITE;
            }
            return true;
        }

        errmsg = info.toString();
        return false;
    }

    void DBClientWithCommands::logout(const string &dbname, BSONObj& info) {
        runCommand(dbname, BSON("logout" << 1), info);
    }

    BSONObj ismastercmdobj = fromjson("{\"ismaster\":1}");

    bool DBClientWithCommands::isMaster(bool& isMaster, BSONObj *info) {
        BSONObj o;
        if ( info == 0 )
            info = &o;
        bool ok = runCommand("admin", ismastercmdobj, *info);
        isMaster = info->getField("ismaster").trueValue();
        return ok;
    }

    bool DBClientWithCommands::createCollection(const string &ns, long long size, bool capped, int max, BSONObj *info) {
        verify(!capped||size);
        BSONObj o;
        if ( info == 0 )    info = &o;
        BSONObjBuilder b;
        string db = nsToDatabase(ns);
        b.append("create", ns.c_str() + db.length() + 1);
        if ( size ) b.append("size", size);
        if ( capped ) b.append("capped", true);
        if ( max ) b.append("max", max);
        return runCommand(db.c_str(), b.done(), *info);
    }

    bool DBClientWithCommands::copyDatabase(const string &fromdb, const string &todb, const string &fromhost, BSONObj *info) {
        BSONObj o;
        if ( info == 0 ) info = &o;
        BSONObjBuilder b;
        b.append("copydb", 1);
        b.append("fromhost", fromhost);
        b.append("fromdb", fromdb);
        b.append("todb", todb);
        return runCommand("admin", b.done(), *info);
    }

    bool DBClientWithCommands::setDbProfilingLevel(const string &dbname, ProfilingLevel level, BSONObj *info ) {
        BSONObj o;
        if ( info == 0 ) info = &o;

        if ( level ) {
            // Create system.profile collection.  If it already exists this does nothing.
            // TODO: move this into the db instead of here so that all
            //       drivers don't have to do this.
            string ns = dbname + ".system.profile";
            createCollection(ns.c_str(), 1024 * 1024, true, 0, info);
        }

        BSONObjBuilder b;
        b.append("profile", (int) level);
        return runCommand(dbname, b.done(), *info);
    }

    BSONObj getprofilingcmdobj = fromjson("{\"profile\":-1}");

    bool DBClientWithCommands::getDbProfilingLevel(const string &dbname, ProfilingLevel& level, BSONObj *info) {
        BSONObj o;
        if ( info == 0 ) info = &o;
        if ( runCommand(dbname, getprofilingcmdobj, *info) ) {
            level = (ProfilingLevel) info->getIntField("was");
            return true;
        }
        return false;
    }

    DBClientWithCommands::MROutput DBClientWithCommands::MRInline (BSON("inline" << 1));

    BSONObj DBClientWithCommands::mapreduce(const string &ns, const string &jsmapf, const string &jsreducef, BSONObj query, MROutput output) {
        BSONObjBuilder b;
        b.append("mapreduce", nsGetCollection(ns));
        b.appendCode("map", jsmapf);
        b.appendCode("reduce", jsreducef);
        if( !query.isEmpty() )
            b.append("query", query);
        b.append("out", output.out);
        BSONObj info;
        runCommand(nsGetDB(ns), b.done(), info);
        return info;
    }

    bool DBClientWithCommands::eval(const string &dbname, const string &jscode, BSONObj& info, BSONElement& retValue, BSONObj *args) {
        BSONObjBuilder b;
        b.appendCode("$eval", jscode);
        if ( args )
            b.appendArray("args", *args);
        bool ok = runCommand(dbname, b.done(), info);
        if ( ok )
            retValue = info.getField("retval");
        return ok;
    }

    bool DBClientWithCommands::eval(const string &dbname, const string &jscode) {
        BSONObj info;
        BSONElement retValue;
        return eval(dbname, jscode, info, retValue);
    }

    list<string> DBClientWithCommands::getDatabaseNames() {
        BSONObj info;
        uassert( 10005 ,  "listdatabases failed" , runCommand( "admin" , BSON( "listDatabases" << 1 ) , info ) );
        uassert( 10006 ,  "listDatabases.databases not array" , info["databases"].type() == Array );

        list<string> names;

        BSONObjIterator i( info["databases"].embeddedObjectUserCheck() );
        while ( i.more() ) {
            names.push_back( i.next().embeddedObjectUserCheck()["name"].valuestr() );
        }

        return names;
    }

    list<string> DBClientWithCommands::getCollectionNames( const string& db ) {
        list<string> names;

        string ns = db + ".system.namespaces";
        auto_ptr<DBClientCursor> c = query( ns.c_str() , BSONObj() );
        while ( c->more() ) {
            string name = c->next()["name"].valuestr();
            if ( name.find( "$" ) != string::npos )
                continue;
            names.push_back( name );
        }
        return names;
    }

    bool DBClientWithCommands::exists( const string& ns ) {
        list<string> names;

        string db = nsGetDB( ns ) + ".system.namespaces";
        BSONObj q = BSON( "name" << ns );
        return count( db.c_str() , q, QueryOption_SlaveOk ) != 0;
    }

    /* --- dbclientconnection --- */

    bool DBClientConnection::auth(const string &dbname, const string &username, const string &password_text, string& errmsg, bool digestPassword, Auth::Level* level) {
        string password = password_text;
        if( digestPassword )
            password = createPasswordDigest( username , password_text );

        if( autoReconnect ) {
            /* note we remember the auth info before we attempt to auth -- if the connection is broken, we will
               then have it for the next autoreconnect attempt.
            */
            pair<string,string> p = pair<string,string>(username, password);
            authCache[dbname] = p;
        }

        return DBClientBase::auth(dbname, username, password.c_str(), errmsg, false, level);
    }

    /** query N objects from the database into an array.  makes sense mostly when you want a small number of results.  if a huge number, use 
        query() and iterate the cursor. 
     */
    void DBClientInterface::findN(vector<BSONObj>& out, const string& ns, Query query, int nToReturn, int nToSkip, const BSONObj *fieldsToReturn, int queryOptions) { 
        out.reserve(nToReturn);

        auto_ptr<DBClientCursor> c =
            this->query(ns, query, nToReturn, nToSkip, fieldsToReturn, queryOptions);

        uassert( 10276 ,  str::stream() << "DBClientBase::findN: transport error: " << getServerAddress() << " ns: " << ns << " query: " << query.toString(), c.get() );

        if ( c->hasResultFlag( ResultFlag_ShardConfigStale ) ){
            BSONObj error;
            c->peekError( &error );
            throw RecvStaleConfigException( "findN stale config", error );
        }

        for( int i = 0; i < nToReturn; i++ ) {
            if ( !c->more() )
                break;
            out.push_back( c->nextSafe().copy() );
        }
    }

    BSONObj DBClientInterface::findOne(const string &ns, const Query& query, const BSONObj *fieldsToReturn, int queryOptions) {
        vector<BSONObj> v;
        findN(v, ns, query, 1, 0, fieldsToReturn, queryOptions);
        return v.empty() ? BSONObj() : v[0];
    }

    bool DBClientConnection::connect(const HostAndPort& server, string& errmsg) {
        _server = server;
        _serverString = _server.toString();
        return _connect( errmsg );
    }
    
#ifdef ROBOMONGO
    bool DBClientConnection::_connect( string& errmsg )
    {
        _serverString = _server.toString();
        server.reset(new mongo::SockAddr(_server.host().c_str(), _server.port()));
#ifdef SSH_SUPPORT_ENABLED
        Robomongo::SSHInfo info = _server.sshInfo();

        if(info.isValid()){
            boost::shared_ptr<mongo::Socket> sock(new Robomongo::SSHSocket(info, _so_timeout, _logLevel ));
            p.reset(new mongo::MessagingPort(sock));
        }
        else
#endif // SSH_SUPPORT_ENABLED
        {
            p.reset(new mongo::MessagingPort( _so_timeout, _logLevel ));
        }

        if (_server.host().empty() || server->getAddr() == "0.0.0.0") {
            return false;
        }

        if ( !p->connect(*server) ) {
            _failed = true;
            return false;
        }
#ifdef MONGO_SSL
        if ( _server.sslInfo()._sslSupport ) {
            const SSLParams params(_server.sslInfo()._sslPEMKeyFile, 
                cmdLine.sslPEMKeyPassword,
                cmdLine.sslCAFile,
                cmdLine.sslCRLFile,
                cmdLine.sslWeakCertificateValidation);
            _sslManager.reset(new SSLManager(params));
            p->secure( _sslManager.get() );
        }
#endif
        return true;
    }

#else

    bool DBClientConnection::_connect( string& errmsg ) {
        _serverString = _server.toString();

        // we keep around SockAddr for connection life -- maybe MessagingPort
        // requires that?
        server.reset(new SockAddr(_server.host().c_str(), _server.port()));
        p.reset(new MessagingPort( _so_timeout, _logLevel ));

        if (_server.host().empty() || server->getAddr() == "0.0.0.0") {
            stringstream s;
            errmsg = 
                str::stream() << "couldn't connect to server " << _server.toString();
            return false;
        }

        // if( _so_timeout == 0 ){
        //    printStackTrace();
        //    log() << "Connecting to server " << _serverString << " timeout " << _so_timeout << endl;
        // }
        if ( !p->connect(*server) ) {
            errmsg = str::stream() << "couldn't connect to server " << _server.toString();
            _failed = true;
            return false;
        }

#ifdef MONGO_SSL
        if ( cmdLine.sslOnNormalPorts ) {
            p->secure( sslManager() );
        }
#endif

        return true;
    }

#endif

    inline bool DBClientConnection::runCommand(const string &dbname,
                                               const BSONObj& cmd,
                                               BSONObj &info,
                                               int options) {
        if (DBClientWithCommands::runCommand(dbname, cmd, info, options))
            return true;
        
        if ( clientSet && isNotMasterErrorString( info["errmsg"] ) ) {
            clientSet->isntMaster();
        }

        return false;
    }


    void DBClientConnection::_checkConnection() {
        if ( !_failed )
            return;
        if ( lastReconnectTry && time(0)-lastReconnectTry < 2 ) {
            // we wait a little before reconnect attempt to avoid constant hammering.
            // but we throw we don't want to try to use a connection in a bad state
            throw SocketException( SocketException::FAILED_STATE , toString() );
        }
        if ( !autoReconnect )
            throw SocketException( SocketException::FAILED_STATE , toString() );

        lastReconnectTry = time(0);
        LOG(_logLevel) << "trying reconnect to " << _serverString << endl;
        string errmsg;
        _failed = false;
        if ( ! _connect(errmsg) ) {
            _failed = true;
            LOG(_logLevel) << "reconnect " << _serverString << " failed " << errmsg << endl;
            throw SocketException( SocketException::CONNECT_ERROR , toString() );
        }

        LOG(_logLevel) << "reconnect " << _serverString << " ok" << endl;
        for( map< string, pair<string,string> >::iterator i = authCache.begin(); i != authCache.end(); i++ ) {
            const char *dbname = i->first.c_str();
            const char *username = i->second.first.c_str();
            const char *password = i->second.second.c_str();
            if( !DBClientBase::auth(dbname, username, password, errmsg, false) )
                LOG(_logLevel) << "reconnect: auth failed db:" << dbname << " user:" << username << ' ' << errmsg << '\n';
        }
    }

    void DBClientConnection::setSoTimeout(double timeout) {
        _so_timeout = timeout;
        if (p) {
            p->setSocketTimeout(timeout);
        }
    }

    uint64_t DBClientConnection::getSockCreationMicroSec() const {
        if (p) {
            return p->getSockCreationMicroSec();
        }
        else {
            return INVALID_SOCK_CREATION_TIME;
        }
    }

    const uint64_t DBClientBase::INVALID_SOCK_CREATION_TIME =
            static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL);

    auto_ptr<DBClientCursor> DBClientBase::query(const string &ns, Query query, int nToReturn,
            int nToSkip, const BSONObj *fieldsToReturn, int queryOptions , int batchSize ) {
        auto_ptr<DBClientCursor> c( new DBClientCursor( this,
                                    ns, query.obj, nToReturn, nToSkip,
                                    fieldsToReturn, queryOptions , batchSize ) );
        if ( c->init() )
            return c;
        return auto_ptr< DBClientCursor >( 0 );
    }

    auto_ptr<DBClientCursor> DBClientBase::getMore( const string &ns, long long cursorId, int nToReturn, int options ) {
        auto_ptr<DBClientCursor> c( new DBClientCursor( this, ns, cursorId, nToReturn, options ) );
        if ( c->init() )
            return c;
        return auto_ptr< DBClientCursor >( 0 );
    }

    struct DBClientFunConvertor {
        void operator()( DBClientCursorBatchIterator &i ) {
            while( i.moreInCurrentBatch() ) {
                _f( i.nextSafe() );
            }
        }
        boost::function<void(const BSONObj &)> _f;
    };

    unsigned long long DBClientBase::query( boost::function<void(const BSONObj&)> f, const string& ns, Query query, const BSONObj *fieldsToReturn, int queryOptions ) {
        DBClientFunConvertor fun;
        fun._f = f;
        boost::function<void(DBClientCursorBatchIterator &)> ptr( fun );
        return this->query( ptr, ns, query, fieldsToReturn, queryOptions );
    }

    unsigned long long DBClientBase::query(
            boost::function<void(DBClientCursorBatchIterator &)> f,
            const string& ns,
            Query query,
            const BSONObj *fieldsToReturn,
            int queryOptions ) {

        // mask options
        queryOptions &= (int)( QueryOption_NoCursorTimeout | QueryOption_SlaveOk );

        auto_ptr<DBClientCursor> c( this->query(ns, query, 0, 0, fieldsToReturn, queryOptions) );
        uassert( 16090, "socket error for mapping query", c.get() );

        unsigned long long n = 0;

        while ( c->more() ) {
            DBClientCursorBatchIterator i( *c );
            f( i );
            n += i.n();
        }
        return n;
    }

    void DBClientConnection::setReplSetClientCallback(DBClientReplicaSet* rsClient) {
        clientSet = rsClient;
    }

    unsigned long long DBClientConnection::query(
            boost::function<void(DBClientCursorBatchIterator &)> f,
            const string& ns,
            Query query,
            const BSONObj *fieldsToReturn,
            int queryOptions ) {

        if ( ! ( availableOptions() & QueryOption_Exhaust ) ) {
            return DBClientBase::query( f, ns, query, fieldsToReturn, queryOptions );
        }

        // mask options
        queryOptions &= (int)( QueryOption_NoCursorTimeout | QueryOption_SlaveOk );
        queryOptions |= (int)QueryOption_Exhaust;

        auto_ptr<DBClientCursor> c( this->query(ns, query, 0, 0, fieldsToReturn, queryOptions) );
        uassert( 13386, "socket error for mapping query", c.get() );

        unsigned long long n = 0;

        try {
            while( 1 ) {
                while( c->moreInCurrentBatch() ) {
                    DBClientCursorBatchIterator i( *c );
                    f( i );
                    n += i.n();
                }

                if( c->getCursorId() == 0 )
                    break;

                c->exhaustReceiveMore();
            }
        }
        catch(std::exception&) {
            /* connection CANNOT be used anymore as more data may be on the way from the server.
               we have to reconnect.
               */
            _failed = true;
            p->shutdown();
            throw;
        }

        return n;
    }

    void DBClientBase::insert( const string & ns , BSONObj obj , int flags) {
        Message toSend;

        BufBuilder b;

        int reservedFlags = 0;
        if( flags & InsertOption_ContinueOnError )
            reservedFlags |= Reserved_InsertOption_ContinueOnError;

        if( flags & WriteOption_FromWriteback )
            reservedFlags |= Reserved_FromWriteback;

        b.appendNum( reservedFlags );
        b.appendStr( ns );
        obj.appendSelfToBufBuilder( b );

        toSend.setData( dbInsert , b.buf() , b.len() );

        say( toSend );
    }

    // TODO: Merge with other insert implementation?
    void DBClientBase::insert( const string & ns , const vector< BSONObj > &v , int flags) {
        Message toSend;

        BufBuilder b;

        int reservedFlags = 0;
        if( flags & InsertOption_ContinueOnError )
            reservedFlags |= Reserved_InsertOption_ContinueOnError;

        if( flags & WriteOption_FromWriteback ){
            reservedFlags |= Reserved_FromWriteback;
            flags ^= WriteOption_FromWriteback;
        }

        b.appendNum( reservedFlags );
        b.appendStr( ns );
        for( vector< BSONObj >::const_iterator i = v.begin(); i != v.end(); ++i )
            i->appendSelfToBufBuilder( b );

        toSend.setData( dbInsert, b.buf(), b.len() );

        say( toSend );
    }

    void DBClientBase::remove( const string & ns , Query obj , bool justOne ) {
        int flags = 0;
        if( justOne ) flags |= RemoveOption_JustOne;
        remove( ns, obj, flags );
    }

    void DBClientBase::remove( const string & ns , Query obj , int flags ) {
        Message toSend;

        BufBuilder b;
        int reservedFlags = 0;
        if( flags & WriteOption_FromWriteback ){
            reservedFlags |= WriteOption_FromWriteback;
            flags ^= WriteOption_FromWriteback;
        }

        b.appendNum( reservedFlags );
        b.appendStr( ns );
        b.appendNum( flags );

        obj.obj.appendSelfToBufBuilder( b );

        toSend.setData( dbDelete , b.buf() , b.len() );

        say( toSend );
    }

    void DBClientBase::update( const string & ns , Query query , BSONObj obj , bool upsert, bool multi ) {
        int flags = 0;
        if ( upsert ) flags |= UpdateOption_Upsert;
        if ( multi ) flags |= UpdateOption_Multi;
        update( ns, query, obj, flags );
    }

    void DBClientBase::update( const string & ns , Query query , BSONObj obj , int flags ) {

        BufBuilder b;

        int reservedFlags = 0;
        if( flags & WriteOption_FromWriteback ){
            reservedFlags |= Reserved_FromWriteback;
            flags ^= WriteOption_FromWriteback;
        }

        b.appendNum( reservedFlags ); // reserved
        b.appendStr( ns );
        b.appendNum( flags );

        query.obj.appendSelfToBufBuilder( b );
        obj.appendSelfToBufBuilder( b );

        Message toSend;
        toSend.setData( dbUpdate , b.buf() , b.len() );

        say( toSend );
    }

    
    auto_ptr<DBClientCursor> DBClientWithCommands::getIndexes( const string &ns ) {
        return query( Namespace( ns.c_str() ).getSisterNS( "system.indexes" ).c_str() , BSON( "ns" << ns ) );
    }

    void DBClientWithCommands::dropIndex( const string& ns , BSONObj keys ) {
        dropIndex( ns , genIndexName( keys ) );
    }


    void DBClientWithCommands::dropIndex( const string& ns , const string& indexName ) {
        BSONObj info;
        if ( ! runCommand( nsToDatabase( ns ) ,
                           BSON( "deleteIndexes" << NamespaceString( ns ).coll << "index" << indexName ) ,
                           info ) ) {
            LOG(_logLevel) << "dropIndex failed: " << info << endl;
            uassert( 10007 ,  "dropIndex failed" , 0 );
        }
        resetIndexCache();
    }

    void DBClientWithCommands::dropIndexes( const string& ns ) {
        BSONObj info;
        uassert( 10008 ,  "dropIndexes failed" , runCommand( nsToDatabase( ns ) ,
                 BSON( "deleteIndexes" << NamespaceString( ns ).coll << "index" << "*") ,
                 info ) );
        resetIndexCache();
    }

    void DBClientWithCommands::reIndex( const string& ns ) {
        list<BSONObj> all;
        auto_ptr<DBClientCursor> i = getIndexes( ns );
        while ( i->more() ) {
            all.push_back( i->next().getOwned() );
        }

        dropIndexes( ns );

        for ( list<BSONObj>::iterator i=all.begin(); i!=all.end(); i++ ) {
            BSONObj o = *i;
            insert( Namespace( ns.c_str() ).getSisterNS( "system.indexes" ).c_str() , o );
        }

    }


    string DBClientWithCommands::genIndexName( const BSONObj& keys ) {
        stringstream ss;

        bool first = 1;
        for ( BSONObjIterator i(keys); i.more(); ) {
            BSONElement f = i.next();

            if ( first )
                first = 0;
            else
                ss << "_";

            ss << f.fieldName() << "_";
            if( f.isNumber() )
                ss << f.numberInt();
            else
                ss << f.str(); //this should match up with shell command
        }
        return ss.str();
    }

    bool DBClientWithCommands::ensureIndex( const string &ns,
                                            BSONObj keys,
                                            bool unique,
                                            const string & name,
                                            bool cache,
                                            bool background,
                                            int version,
                                            int ttl ) {
        BSONObjBuilder toSave;
        toSave.append( "ns" , ns );
        toSave.append( "key" , keys );

        string cacheKey(ns);
        cacheKey += "--";

        if ( name != "" ) {
            toSave.append( "name" , name );
            cacheKey += name;
        }
        else {
            string nn = genIndexName( keys );
            toSave.append( "name" , nn );
            cacheKey += nn;
        }

        if( version >= 0 ) 
            toSave.append("v", version);

        if ( unique )
            toSave.appendBool( "unique", unique );

        if( background ) 
            toSave.appendBool( "background", true );

        if ( _seenIndexes.count( cacheKey ) )
            return 0;

        if ( cache )
            _seenIndexes.insert( cacheKey );

        if ( ttl > 0 )
            toSave.append( "expireAfterSeconds", ttl );

        insert( Namespace( ns.c_str() ).getSisterNS( "system.indexes"  ).c_str() , toSave.obj() );
        return 1;
    }

    void DBClientWithCommands::resetIndexCache() {
        _seenIndexes.clear();
    }

    /* -- DBClientCursor ---------------------------------------------- */

#ifdef _DEBUG
#define CHECK_OBJECT( o , msg ) massert( 10337 ,  (string)"object not valid" + (msg) , (o).isValid() )
#else
#define CHECK_OBJECT( o , msg )
#endif

    void assembleRequest( const string &ns, BSONObj query, int nToReturn, int nToSkip, const BSONObj *fieldsToReturn, int queryOptions, Message &toSend ) {
        CHECK_OBJECT( query , "assembleRequest query" );
        // see query.h for the protocol we are using here.
        BufBuilder b;
        int opts = queryOptions;
        b.appendNum(opts);
        b.appendStr(ns);
        b.appendNum(nToSkip);
        b.appendNum(nToReturn);
        query.appendSelfToBufBuilder(b);
        if ( fieldsToReturn )
            fieldsToReturn->appendSelfToBufBuilder(b);
        toSend.setData(dbQuery, b.buf(), b.len());
    }

    void DBClientConnection::say( Message &toSend, bool isRetry , string * actualServer ) {
        checkConnection();
        try {
            port().say( toSend );
        }
        catch( SocketException & ) {
            _failed = true;
            throw;
        }
    }

    void DBClientConnection::sayPiggyBack( Message &toSend ) {
        port().piggyBack( toSend );
    }

    bool DBClientConnection::recv( Message &m ) {
        if (port().recv(m)) {
            return true;
        }

        _failed = true;
        return false;
    }

    bool DBClientConnection::call( Message &toSend, Message &response, bool assertOk , string * actualServer ) {
        /* todo: this is very ugly messagingport::call returns an error code AND can throw
                 an exception.  we should make it return void and just throw an exception anytime
                 it fails
        */
        checkConnection();
        try {
            if ( !port().call(toSend, response) ) {
                _failed = true;
                if ( assertOk )
                    uasserted( 10278 , str::stream() << "dbclient error communicating with server: " << getServerAddress() );

                return false;
            }
        }
        catch( SocketException & ) {
            _failed = true;
            throw;
        }
        return true;
    }

    BSONElement getErrField(const BSONObj& o) {
        BSONElement first = o.firstElement();
        if( strcmp(first.fieldName(), "$err") == 0 )
            return first;

        // temp - will be DEV only later
        /*DEV*/ 
        if( 1 ) {
            BSONElement e = o["$err"];
            if( !e.eoo() ) { 
                wassert(false);
            }
            return e;
        }

        return BSONElement();
    }

    bool hasErrField( const BSONObj& o ){
        return ! getErrField( o ).eoo();
    }

    void DBClientConnection::checkResponse( const char *data, int nReturned, bool* retry, string* host ) {
        /* check for errors.  the only one we really care about at
         * this stage is "not master" 
        */
        
        *retry = false;
        *host = _serverString;

        if ( clientSet && nReturned ) {
            verify(data);
            BSONObj o(data);
            if ( isNotMasterErrorString( getErrField(o) ) ) {
                clientSet->isntMaster();
            }
        }
    }

    void DBClientConnection::killCursor( long long cursorId ) {
        StackBufBuilder b;
        b.appendNum( (int)0 ); // reserved
        b.appendNum( (int)1 ); // number
        b.appendNum( cursorId );

        Message m;
        m.setData( dbKillCursors , b.buf() , b.len() );
        
        if ( _lazyKillCursor )
            sayPiggyBack( m );
        else
            say(m);
    }

#ifdef MONGO_SSL
    static SimpleMutex s_mtx("SSLManager");
    static SSLManager* s_sslMgr(NULL);

    SSLManager* DBClientConnection::sslManager() {
        SimpleMutex::scoped_lock lk(s_mtx);
        if (s_sslMgr) 
            return s_sslMgr;
        const SSLParams params(cmdLine.sslPEMKeyFile, 
                               cmdLine.sslPEMKeyPassword,
                               cmdLine.sslCAFile,
                               cmdLine.sslCRLFile,
                               cmdLine.sslWeakCertificateValidation);
        s_sslMgr = new SSLManager(params);
        

        return s_sslMgr;
    }
#endif

    AtomicUInt DBClientConnection::_numConnections;
    bool DBClientConnection::_lazyKillCursor = true;


    bool serverAlive( const string &uri ) {
        DBClientConnection c( false, 0, 20 ); // potentially the connection to server could fail while we're checking if it's alive - so use timeouts
        string err;
        if ( !c.connect( uri, err ) )
            return false;
        if ( !c.simpleCommand( "admin", 0, "ping" ) )
            return false;
        return true;
    }


    /** @return the database name portion of an ns string */
    string nsGetDB( const string &ns ) {
        string::size_type pos = ns.find( "." );
        if ( pos == string::npos )
            return ns;

        return ns.substr( 0 , pos );
    }

    /** @return the collection name portion of an ns string */
    string nsGetCollection( const string &ns ) {
        string::size_type pos = ns.find( "." );
        if ( pos == string::npos )
            return "";

        return ns.substr( pos + 1 );
    }


} // namespace mongo
