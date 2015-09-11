/** @file dbclient_rs.h Connect to a Replica Set, from C++ */

/*    Copyright 2009 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <utility>

#include "mongo/client/dbclientinterface.h"
#include "mongo/client/export_macros.h"
#include "mongo/util/net/hostandport.h"

namespace mongo {

class ReplicaSetMonitor;
class TagSet;
struct ReadPreferenceSetting;
typedef boost::shared_ptr<ReplicaSetMonitor> ReplicaSetMonitorPtr;

/** Use this class to connect to a replica set of servers.  The class will manage
   checking for which server in a replica set is master, and do failover automatically.

   This can also be used to connect to replica pairs since pairs are a subset of sets

   On a failover situation, expect at least one operation to return an error (throw
   an exception) before the failover is complete.  Operations are not retried.
*/
class MONGO_CLIENT_API DBClientReplicaSet : public DBClientBase {
public:
    using DBClientBase::query;
    using DBClientBase::update;
    using DBClientBase::remove;

    /** Call connect() after constructing. autoReconnect is always on for DBClientReplicaSet
     * connections. */
    DBClientReplicaSet(const std::string& name,
                       const std::vector<HostAndPort>& servers,
                       double so_timeout = 0);
    virtual ~DBClientReplicaSet();

    /**
     * Returns false if no member of the set were reachable. This object
     * can still be used even when false was returned as it will try to
     * reconnect when you use it later.
     */
    bool connect();

    /**
     * Logs out the connection for the given database.
     *
     * @param dbname the database to logout from.
     * @param info the result object for the logout command (provided for backwards
     *     compatibility with mongo shell)
     */
    virtual void logout(const std::string& dbname, BSONObj& info);

    // ----------- simple functions --------------

    /** throws userassertion "no master found" */
    virtual std::auto_ptr<DBClientCursor> query(const std::string& ns,
                                                Query query,
                                                int nToReturn = 0,
                                                int nToSkip = 0,
                                                const BSONObj* fieldsToReturn = 0,
                                                int queryOptions = 0,
                                                int batchSize = 0);

    /** throws userassertion "no master found" */
    virtual BSONObj findOne(const std::string& ns,
                            const Query& query,
                            const BSONObj* fieldsToReturn = 0,
                            int queryOptions = 0);

    virtual void insert(const std::string& ns, BSONObj obj, int flags = 0);

    /** insert multiple objects.  Note that single object insert is asynchronous, so this version
        is only nominally faster and not worth a special effort to try to use.  */
    virtual void insert(const std::string& ns, const std::vector<BSONObj>& v, int flags = 0);

    virtual void remove(const std::string& ns, Query obj, int flags);

    virtual void update(const std::string& ns, Query query, BSONObj obj, int flags);

    virtual void killCursor(long long cursorID);

    // ---- access raw connections ----

    /**
     * WARNING: this method is very dangerous - this object can decide to free the
     *     returned master connection any time.
     *
     * @return the reference to the address that points to the master connection.
     */
    DBClientConnection& masterConn();

    /**
     * WARNING: this method is very dangerous - this object can decide to free the
     *     returned master connection any time. This can also unpin the cached
     *     slaveOk/read preference connection.
     *
     * @return the reference to the address that points to a secondary connection.
     */
    DBClientConnection& slaveConn();

    // ---- callback pieces -------

    virtual void say(Message& toSend, bool isRetry = false, std::string* actualServer = 0);
    virtual bool recv(Message& toRecv);
    virtual void checkResponse(const char* data,
                               int nReturned,
                               bool* retry = NULL,
                               std::string* targetHost = NULL);

    /* this is the callback from our underlying connections to notify us that we got a "not master"
     * error.
     */
    void isntMaster();

    /* this is used to indicate we got a "not master or secondary" error from a secondary.
     */
    void isntSecondary();

    // ----- status ------

    virtual bool isFailed() const {
        return !_master || _master->isFailed();
    }
    bool isStillConnected();

    // ----- informational ----

    /**
     * Gets the replica set name of the set we are connected to.
     */
    const std::string& getSetName() const {
        return _setName;
    }

    /**
     * Returns the HostAndPort of the server this connection believes belongs to the primary,
     * or returns an empty HostAndPort if it doesn't know about a current primary.
     */
    HostAndPort getSuspectedPrimaryHostAndPort() const;

    double getSoTimeout() const {
        return _so_timeout;
    }

    std::string toString() const {
        return getServerAddress();
    }

    std::string getServerAddress() const;

    virtual ConnectionString::ConnectionType type() const {
        return ConnectionString::SET;
    }
    virtual bool lazySupported() const {
        return true;
    }

    // ---- low level ------

    virtual bool call(Message& toSend,
                      Message& response,
                      bool assertOk = true,
                      std::string* actualServer = 0);
    virtual bool callRead(Message& toSend, Message& response) {
        return checkMaster()->callRead(toSend, response);
    }

    /**
     * Returns whether a query or command can be sent to secondaries based on the query object
     * and options.
     *
     * @param ns the namespace of the query.
     * @param queryObj the query object to check.
     * @param queryOptions the query options
     *
     * @return true if the query/cmd could potentially be sent to a secondary, false otherwise
     */
    static bool isSecondaryQuery(const std::string& ns, const BSONObj& queryObj, int queryOptions);

    virtual void setRunCommandHook(DBClientWithCommands::RunCommandHookFunc func);
    virtual void setPostRunCommandHook(DBClientWithCommands::PostRunCommandHookFunc func);

    /**
     * Performs a "soft reset" by clearing all states relating to secondary nodes and
     * returning secondary connections to the pool.
     */
    virtual void reset();

    /**
     * @bool setting if true, DBClientReplicaSet connections will make sure that secondary
     *    connections are authenticated and log them before returning them to the pool.
     */
    static void setAuthPooledSecondaryConn(bool setting);

protected:
    /** Authorize.  Authorizes all nodes as needed
    */
    virtual void _auth(const BSONObj& params);

    virtual void sayPiggyBack(Message& toSend) {
        checkMaster()->say(toSend);
    }

private:
    /**
     * Used to simplify slave-handling logic on errors
     *
     * @return back the passed cursor
     * @throws DBException if the directed node cannot accept the query because it
     *     is not a master
     */
    std::auto_ptr<DBClientCursor> checkSlaveQueryResult(std::auto_ptr<DBClientCursor> result);

    DBClientConnection* checkMaster();

    /**
     * Helper method for selecting a node based on the read preference. Will advance
     * the tag tags object if it cannot find a node that matches the current tag.
     *
     * @param readPref the preference to use for selecting a node.
     *
     * @return a pointer to the new connection object if it can find a good connection.
     *     Otherwise it returns NULL.
     *
     * @throws DBException when an error occurred either when trying to connect to
     *     a node that was thought to be ok or when an assertion happened.
     */
    DBClientConnection* selectNodeUsingTags(boost::shared_ptr<ReadPreferenceSetting> readPref);

    /**
     * @return true if the last host used in the last slaveOk query is still in the
     * set and can be used for the given read preference.
     */
    bool checkLastHost(const ReadPreferenceSetting* readPref);

    /**
     * Destroys all cached information about the last slaveOk operation.
     */
    void invalidateLastSlaveOkCache();

    void _auth(DBClientConnection* conn);

    /**
     * Calls logout on the connection for all known database this DBClientRS instance has
     * logged in.
     */
    void logoutAll(DBClientConnection* conn);

    /**
     * Clears the master connection.
     */
    void resetMaster();

    /**
     * Clears the slaveOk connection and returns it to the pool if not the same as _master.
     */
    void resetSlaveOkConn();

    /**
     * Maximum number of retries to make for auto-retry logic when performing a slave ok
     * operation.
     */
    static const size_t MAX_RETRY;

    // TODO: remove this when processes other than mongos uses the driver version.
    static bool _authPooledSecondaryConn;

    // Throws a DBException if the monitor doesn't exist and there isn't a cached seed to use.
    ReplicaSetMonitorPtr _getMonitor() const;

    std::string _setName;

    HostAndPort _masterHost;
    boost::scoped_ptr<DBClientConnection> _master;

    // Last used host in a slaveOk query (can be a primary).
    HostAndPort _lastSlaveOkHost;
    // Last used connection in a slaveOk query (can be a primary).
    // Connection can either be owned here or returned to the connection pool. Note that
    // if connection is primary, it is owned by _master so it is incorrect to return
    // it to the pool.
    std::auto_ptr<DBClientConnection> _lastSlaveOkConn;
    boost::shared_ptr<ReadPreferenceSetting> _lastReadPref;

    double _so_timeout;

    // we need to store so that when we connect to a new node on failure
    // we can re-auth
    // this could be a security issue, as the password is stored in memory
    // not sure if/how we should handle
    std::map<std::string, BSONObj> _auths;  // dbName -> auth parameters

protected:
    /**
     * for storing (non-threadsafe) information between lazy calls
     */
    class LazyState {
    public:
        LazyState() : _lastClient(NULL), _lastOp(-1), _secondaryQueryOk(false), _retries(0) {}
        DBClientConnection* _lastClient;
        int _lastOp;
        bool _secondaryQueryOk;
        int _retries;

    } _lazyState;
};

/**
 * A simple object for representing the list of tags requested by a $readPreference.
 */
class MONGO_CLIENT_API TagSet {
public:
    /**
     * Creates a TagSet that matches any nodes.
     *
     * Do not call during static init.
     */
    TagSet();

    /**
     * Creates a TagSet from a BSONArray of tags.
     *
     * @param tags the list of tags associated with this option. This object
     *     will get a shared copy of the list. Therefore, it is important
     *     for the the given tag to live longer than the created tag set.
     */
    explicit TagSet(const BSONArray& tags) : _tags(tags) {}

    /**
     * Returns the BSONArray listing all tags that should be accepted.
     */
    const BSONArray& getTagBSON() const {
        return _tags;
    }

    bool operator==(const TagSet& other) const {
        return _tags == other._tags;
    }

private:
    BSONArray _tags;
};

struct MONGO_CLIENT_API ReadPreferenceSetting {
    /**
     * @parm pref the read preference mode.
     * @param tag the tag set. Note that this object will have the
     *     tag set will have this in a reset state (meaning, this
     *     object's copy of tag will have the iterator in the initial
     *     position).
     */
    ReadPreferenceSetting(ReadPreference pref, const TagSet& tag) : pref(pref), tags(tag) {}

    inline bool equals(const ReadPreferenceSetting& other) const {
        return pref == other.pref && tags == other.tags;
    }

    BSONObj toBSON() const;

    const ReadPreference pref;
    TagSet tags;
};
}
