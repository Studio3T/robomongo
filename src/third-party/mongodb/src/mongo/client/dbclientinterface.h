/** @file dbclientinterface.h

    Core MongoDB C++ driver interfaces are defined here.
*/

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

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "mongo/base/string_data.h"
#include "mongo/bson/bson_field.h"
#include "mongo/client/export_macros.h"
#include "mongo/db/jsobj.h"
#include "mongo/logger/log_severity.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/stdx/functional.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/net/message.h"
#include "mongo/util/net/message_port.h"

namespace mongo {

/** the query field 'options' can have these bits set: */
enum MONGO_CLIENT_API QueryOptions {
    /** Tailable means cursor is not closed when the last data is retrieved.  rather, the cursor
     * marks the final object's position.  you can resume using the cursor later, from where it was
       located, if more data were received.  Set on dbQuery and dbGetMore.

       like any "latent cursor", the cursor may become invalid at some point -- for example if that
       final object it references were deleted.  Thus, you should be prepared to requery if you get
       back ResultFlag_CursorNotFound.
    */
    QueryOption_CursorTailable = 1 << 1,

    /** allow query of replica slave.  normally these return an error except for namespace "local".
    */
    QueryOption_SlaveOk = 1 << 2,

    // findingStart mode is used to find the first operation of interest when
    // we are scanning through a repl log.  For efficiency in the common case,
    // where the first operation of interest is closer to the tail than the head,
    // we start from the tail of the log and work backwards until we find the
    // first operation of interest.  Then we scan forward from that first operation,
    // actually returning results to the client.  During the findingStart phase,
    // we release the db mutex occasionally to avoid blocking the db process for
    // an extended period of time.
    QueryOption_OplogReplay = 1 << 3,

    /** The server normally times out idle cursors after an inactivity period to prevent excess
     * memory uses
        Set this option to prevent that.
    */
    QueryOption_NoCursorTimeout = 1 << 4,

    /** Use with QueryOption_CursorTailable.  If we are at the end of the data, block for a while
     * rather than returning no data. After a timeout period, we do return as normal.
    */
    QueryOption_AwaitData = 1 << 5,

    /** Stream the data down full blast in multiple "more" packages, on the assumption that the
     * client will fully read all data queried.  Faster when you are pulling a lot of data and know
     * you want to pull it all down.  Note: it is not allowed to not read all the data unless you
     * close the connection.

        Use the query( stdx::function<void(const BSONObj&)> f, ... ) version of the connection's
        query()
        method, and it will take care of all the details for you.
    */
    QueryOption_Exhaust = 1 << 6,

    /** When sharded, this means its ok to return partial results
        Usually we will fail a query if all required shards aren't up
        If this is set, it'll be a partial result set
     */
    QueryOption_PartialResults = 1 << 7,

    QueryOption_AllSupported = QueryOption_CursorTailable | QueryOption_SlaveOk |
        QueryOption_OplogReplay | QueryOption_NoCursorTimeout | QueryOption_AwaitData |
        QueryOption_Exhaust | QueryOption_PartialResults,

    QueryOption_AllSupportedForSharding = QueryOption_CursorTailable | QueryOption_SlaveOk |
        QueryOption_OplogReplay | QueryOption_NoCursorTimeout | QueryOption_AwaitData |
        QueryOption_PartialResults,
};

enum MONGO_CLIENT_API UpdateOptions {
    /** Upsert - that is, insert the item if no matching item is found. */
    UpdateOption_Upsert = 1 << 0,

    /** Update multiple documents (if multiple documents match query expression).
       (Default is update a single document and stop.) */
    UpdateOption_Multi = 1 << 1,

    /** flag from mongo saying this update went everywhere */
    UpdateOption_Broadcast = 1 << 2
};

enum MONGO_CLIENT_API RemoveOptions {
    /** only delete one option */
    RemoveOption_JustOne = 1 << 0,

    /** flag from mongo saying this update went everywhere */
    RemoveOption_Broadcast = 1 << 1
};


/**
 * need to put in DbMesssage::ReservedOptions as well
 */
enum MONGO_CLIENT_API InsertOptions {
    /** With muli-insert keep processing inserts if one fails */
    InsertOption_ContinueOnError = 1 << 0
};

/**
 * Start from *top* of bits, these are generic write options that apply to all
 */
enum MONGO_CLIENT_API WriteOptions {
    /** logical writeback option */
    WriteOption_FromWriteback = 1 << 31
};

//
// For legacy reasons, the reserved field pre-namespace of certain types of messages is used
// to store options as opposed to the flags after the namespace.  This should be transparent to
// the api user, but we need these constants to disassemble/reassemble the messages correctly.
//

enum MONGO_CLIENT_API ReservedOptions {
    Reserved_InsertOption_ContinueOnError = 1 << 0,
    Reserved_FromWriteback = 1 << 1
};

enum MONGO_CLIENT_API ReadPreference {
    /**
     * Read from primary only. All operations produce an error (throw an
     * exception where applicable) if primary is unavailable. Cannot be
     * combined with tags.
     */
    ReadPreference_PrimaryOnly = 0,

    /**
     * Read from primary if available, otherwise a secondary. Tags will
     * only be applied in the event that the primary is unavailable and
     * a secondary is read from. In this event only secondaries matching
     * the tags provided would be read from.
     */
    ReadPreference_PrimaryPreferred,

    /**
     * Read from secondary if available, otherwise error.
     */
    ReadPreference_SecondaryOnly,

    /**
     * Read from a secondary if available, otherwise read from the primary.
     */
    ReadPreference_SecondaryPreferred,

    /**
     * Read from any member.
     */
    ReadPreference_Nearest,
};

class MONGO_CLIENT_API DBClientBase;
class MONGO_CLIENT_API DBClientConnection;

/**
 * ConnectionString handles parsing different ways to connect to mongo and determining method
 * samples:
 *    server
 *    server:port
 *    foo/server:port,server:port   SET
 *    server,server,server          SYNC
 *                                    Warning - you usually don't want "SYNC", it's used
 *                                    for some special things such as sharding config servers.
 *                                    See syncclusterconnection.h for more info.
 *
 * tyipcal use
 * std::string errmsg,
 * ConnectionString cs = ConnectionString::parse( url , errmsg );
 * if ( ! cs.isValid() ) throw "bad: " + errmsg;
 * DBClientBase * conn = cs.connect( errmsg );
 */
class MONGO_CLIENT_API ConnectionString {
public:
    enum ConnectionType { INVALID, MASTER, PAIR, SET, SYNC, CUSTOM };

    ConnectionString() {
        _type = INVALID;
    }

    // Note: This should only be used for direct connections to a single server.  For replica
    // set and SyncClusterConnections, use ConnectionString::parse.
    ConnectionString(const HostAndPort& server) {
        _type = MASTER;
        _servers.push_back(server);
        _finishInit();
    }

    ConnectionString(ConnectionType type, const std::string& s, const std::string& setName = "") {
        _type = type;
        _setName = setName;
        _fillServers(s);

        switch (_type) {
            case MASTER:
                verify(_servers.size() == 1);
                break;
            case SET:
                verify(_setName.size());
                verify(_servers.size() >= 1);  // 1 is ok since we can derive
                break;
            case PAIR:
                verify(_servers.size() == 2);
                break;
            default:
                verify(_servers.size() > 0);
        }

        _finishInit();
    }

    ConnectionString(const std::string& s, ConnectionType favoredMultipleType) {
        _type = INVALID;

        _fillServers(s);
        if (_type != INVALID) {
            // set already
        } else if (_servers.size() == 1) {
            _type = MASTER;
        } else {
            _type = favoredMultipleType;
            verify(_type == SET || _type == SYNC);
        }
        _finishInit();
    }

    bool isValid() const {
        return _type != INVALID;
    }

    std::string toString() const {
        return _string;
    }

    DBClientBase* connect(std::string& errmsg, double socketTimeout = 0) const;

    std::string getSetName() const {
        return _setName;
    }

    const std::vector<HostAndPort>& getServers() const {
        return _servers;
    }

    ConnectionType type() const {
        return _type;
    }

    /**
     * This returns true if this and other point to the same logical entity.
     * For single nodes, thats the same address.
     * For replica sets, thats just the same replica set name.
     * For pair (deprecated) or sync cluster connections, that's the same hosts in any ordering.
     */
    bool sameLogicalEndpoint(const ConnectionString& other) const;

    static ConnectionString parse(const std::string& url, std::string& errmsg);

    static std::string typeToString(ConnectionType type);

    //
    // Allow overriding the default connection behavior
    // This is needed for some tests, which otherwise would fail because they are unable to contact
    // the correct servers.
    //

    class ConnectionHook {
    public:
        virtual ~ConnectionHook() {}

        // Returns an alternative connection object for a string
        virtual DBClientBase* connect(const ConnectionString& c,
                                      std::string& errmsg,
                                      double socketTimeout) = 0;
    };

    static void setConnectionHook(ConnectionHook* hook) {
        scoped_lock lk(_connectHookMutex);
        _connectHook = hook;
    }

    static ConnectionHook* getConnectionHook() {
        scoped_lock lk(_connectHookMutex);
        return _connectHook;
    }

    // Allows ConnectionStrings to be stored more easily in sets/maps
    bool operator<(const ConnectionString& other) const {
        return _string < other._string;
    }

    //
    // FOR TESTING ONLY - useful to be able to directly mock a connection std::string without
    // including the entire client library.
    //

    static ConnectionString mock(const HostAndPort& server) {
        ConnectionString connStr;
        connStr._servers.push_back(server);
        connStr._string = server.toString();
        return connStr;
    }

private:
    void _fillServers(std::string s);
    void _finishInit();

    ConnectionType _type;
    std::vector<HostAndPort> _servers;
    std::string _string;
    std::string _setName;

    static mutex _connectHookMutex;
    static ConnectionHook* _connectHook;
};

/**
 * controls how much a clients cares about writes
 * default is NORMAL
 */
enum MONGO_CLIENT_API WriteConcern {
    W_NONE = 0,  // TODO: not every connection type fully supports this
    W_NORMAL = 1
    // TODO SAFE = 2
};

class BSONObj;
class ScopedDbConnection;
class DBClientCursor;
class DBClientCursorBatchIterator;

/** Represents a Mongo query expression.  Typically one uses the QUERY(...) macro to construct a
 * Query object.
    Examples:
       QUERY( "age" << 33 << "school" << "UCLA" ).sort("name")
       QUERY( "age" << GT << 30 << LT << 50 )
*/
class MONGO_CLIENT_API Query {
public:
    static const BSONField<BSONObj> ReadPrefField;
    static const BSONField<std::string> ReadPrefModeField;
    static const BSONField<BSONArray> ReadPrefTagsField;

    BSONObj obj;
    Query() : obj(BSONObj()) {}
    Query(const BSONObj& b) : obj(b) {}
    Query(const std::string& json);
    Query(const char* json);

    /** Add a sort (ORDER BY) criteria to the query expression.
        @param sortPattern the sort order template.  For example to order by name ascending, time
        descending:
          { name : 1, ts : -1 }
        i.e.
          BSON( "name" << 1 << "ts" << -1 )
        or
          fromjson(" name : 1, ts : -1 ")
    */
    Query& sort(const BSONObj& sortPattern);

    /** Add a sort (ORDER BY) criteria to the query expression.
        This version of sort() assumes you want to sort on a single field.
        @param asc = 1 for ascending order
        asc = -1 for descending order
    */
    Query& sort(const std::string& field, int asc = 1) {
        sort(BSON(field << asc));
        return *this;
    }

    /** Provide a hint to the query.
        @param keyPattern Key pattern for the index to use.
        Example:
          hint("{ts:1}")
    */
    Query& hint(BSONObj keyPattern);
    Query& hint(const std::string& jsonKeyPatt);

    /** Provide min and/or max index limits for the query.
        min <= x < max
     */
    Query& minKey(const BSONObj& val);
    /**
       max is exclusive
     */
    Query& maxKey(const BSONObj& val);

    /** Return explain information about execution of this query instead of the actual query
     * results.
        Normally it is easier to use the mongo shell to run db.find(...).explain().
    */
    Query& explain();

    /** Use snapshot mode for the query.  Snapshot mode assures no duplicates are returned, or
     * objects missed, which were present at both the start and end of the query's execution (if an
     * object is new during the query, or deleted during the query, it may or may not be returned,
     * even with snapshot mode).

        Note that short query responses (less than 1MB) are always effectively snapshotted.

        Currently, snapshot mode may not be used with sorting or explicit hints.
    */
    Query& snapshot();

    /** Queries to the Mongo database support a $where parameter option which contains
        a javascript function that is evaluated to see whether objects being queried match
        its criteria.  Use this helper to append such a function to a query object.
        Your query may also contain other traditional Mongo query terms.

        @param jscode The javascript function to evaluate against each potential object
               match.  The function must return true for matched objects.  Use the this
               variable to inspect the current object.
        @param scope SavedContext for the javascript object.  List in a BSON object any
               variables you would like defined when the jscode executes.  One can think
               of these as "bind variables".

        Examples:
          conn.findOne("test.coll", Query("{a:3}").where("this.b == 2 || this.c == 3"));
          Query badBalance = Query().where("this.debits - this.credits < 0");
    */
    Query& where(const std::string& jscode, BSONObj scope);
    Query& where(const std::string& jscode) {
        return where(jscode, BSONObj());
    }

    /**
     * Sets the read preference for this query.
     *
     * @param pref the read preference mode for this query.
     * @param tags the set of tags to use for this query.
     */
    Query& readPref(ReadPreference pref, const BSONArray& tags);

    /**
     * @return true if this query has an orderby, hint, or some other field
     */
    bool isComplex(bool* hasDollar = 0) const;
    static bool isComplex(const BSONObj& obj, bool* hasDollar = 0);

    BSONObj getFilter() const;
    BSONObj getSort() const;
    BSONObj getHint() const;
    bool isExplain() const;

    /**
     * @return true if the query object contains a read preference specification object.
     */
    static bool hasReadPreference(const BSONObj& queryObj);

    std::string toString() const;
    operator std::string() const {
        return toString();
    }

private:
    void makeComplex();
    template <class T>
    void appendComplex(const char* fieldName, const T& val) {
        makeComplex();
        BSONObjBuilder b;
        b.appendElements(obj);
        b.append(fieldName, val);
        obj = b.obj();
    }
};

/**
 * Represents a full query description, including all options required for the query to be passed on
 * to other hosts
 */
class MONGO_CLIENT_API QuerySpec {
    std::string _ns;
    int _ntoskip;
    int _ntoreturn;
    int _options;
    BSONObj _query;
    BSONObj _fields;
    Query _queryObj;

public:
    QuerySpec(const std::string& ns,
              const BSONObj& query,
              const BSONObj& fields,
              int ntoskip,
              int ntoreturn,
              int options)
        : _ns(ns),
          _ntoskip(ntoskip),
          _ntoreturn(ntoreturn),
          _options(options),
          _query(query.getOwned()),
          _fields(fields.getOwned()),
          _queryObj(_query) {}

    QuerySpec() {}

    bool isEmpty() const {
        return _ns.size() == 0;
    }

    bool isExplain() const {
        return _queryObj.isExplain();
    }
    BSONObj filter() const {
        return _queryObj.getFilter();
    }

    BSONObj hint() const {
        return _queryObj.getHint();
    }
    BSONObj sort() const {
        return _queryObj.getSort();
    }
    BSONObj query() const {
        return _query;
    }
    BSONObj fields() const {
        return _fields;
    }
    BSONObj* fieldsData() {
        return &_fields;
    }

    // don't love this, but needed downstrem
    const BSONObj* fieldsPtr() const {
        return &_fields;
    }

    std::string ns() const {
        return _ns;
    }
    int ntoskip() const {
        return _ntoskip;
    }
    int ntoreturn() const {
        return _ntoreturn;
    }
    int options() const {
        return _options;
    }

    void setFields(BSONObj& o) {
        _fields = o.getOwned();
    }

    std::string toString() const {
        return str::stream() << "QSpec " << BSON("ns" << _ns << "n2skip" << _ntoskip << "n2return"
                                                      << _ntoreturn << "options" << _options
                                                      << "query" << _query << "fields" << _fields);
    }
};


/** Typically one uses the QUERY(...) macro to construct a Query object.
    Example: QUERY( "age" << 33 << "school" << "UCLA" )
*/
#define QUERY(x) ::mongo::Query(BSON(x))

// Useful utilities for namespaces
/** @return the database name portion of an ns std::string */
MONGO_CLIENT_API std::string nsGetDB(const std::string& ns);

/** @return the collection name portion of an ns std::string */
MONGO_CLIENT_API std::string nsGetCollection(const std::string& ns);

/**
   interface that handles communication with the db
 */
class MONGO_CLIENT_API DBConnector {
public:
    virtual ~DBConnector() {}
    /** actualServer is set to the actual server where they call went if there was a choice
     * (SlaveOk) */
    virtual bool call(Message& toSend,
                      Message& response,
                      bool assertOk = true,
                      std::string* actualServer = 0) = 0;
    virtual void say(Message& toSend, bool isRetry = false, std::string* actualServer = 0) = 0;
    virtual void sayPiggyBack(Message& toSend) = 0;
    /* used by QueryOption_Exhaust.  To use that your subclass must implement this. */
    virtual bool recv(Message& m) {
        verify(false);
        return false;
    }
    // In general, for lazy queries, we'll need to say, recv, then checkResponse
    virtual void checkResponse(const char* data,
                               int nReturned,
                               bool* retry = NULL,
                               std::string* targetHost = NULL) {
        if (retry)
            *retry = false;
        if (targetHost)
            *targetHost = "";
    }
    virtual bool lazySupported() const = 0;
};

/**
   The interface that any db connection should implement
 */
class MONGO_CLIENT_API DBClientInterface : boost::noncopyable {
public:
    virtual std::auto_ptr<DBClientCursor> query(const std::string& ns,
                                                Query query,
                                                int nToReturn = 0,
                                                int nToSkip = 0,
                                                const BSONObj* fieldsToReturn = 0,
                                                int queryOptions = 0,
                                                int batchSize = 0) = 0;

    virtual void insert(const std::string& ns, BSONObj obj, int flags = 0) = 0;

    virtual void insert(const std::string& ns, const std::vector<BSONObj>& v, int flags = 0) = 0;

    virtual void remove(const std::string& ns, Query query, bool justOne = 0) = 0;

    virtual void remove(const std::string& ns, Query query, int flags) = 0;

    virtual void update(const std::string& ns,
                        Query query,
                        BSONObj obj,
                        bool upsert = false,
                        bool multi = false) = 0;

    virtual void update(const std::string& ns, Query query, BSONObj obj, int flags) = 0;

    virtual ~DBClientInterface() {}

    /**
       @return a single object that matches the query.  if none do, then the object is empty
       @throws AssertionException
    */
    virtual BSONObj findOne(const std::string& ns,
                            const Query& query,
                            const BSONObj* fieldsToReturn = 0,
                            int queryOptions = 0);

    /** query N objects from the database into an array.  makes sense mostly when you want a small
     * number of results.  if a huge number, use
        query() and iterate the cursor.
    */
    void findN(std::vector<BSONObj>& out,
               const std::string& ns,
               Query query,
               int nToReturn,
               int nToSkip = 0,
               const BSONObj* fieldsToReturn = 0,
               int queryOptions = 0);

    virtual std::string getServerAddress() const = 0;

    /** don't use this - called automatically by DBClientCursor for you */
    virtual std::auto_ptr<DBClientCursor> getMore(const std::string& ns,
                                                  long long cursorId,
                                                  int nToReturn = 0,
                                                  int options = 0) = 0;
};

/**
   DB "commands"
   Basically just invocations of connection.$cmd.findOne({...});
*/
class MONGO_CLIENT_API DBClientWithCommands : public DBClientInterface {
    std::set<std::string> _seenIndexes;

public:
    /** controls how chatty the client is about network errors & such.  See log.h */
    logger::LogSeverity _logLevel;

    DBClientWithCommands()
        : _logLevel(logger::LogSeverity::Log()),
          _cachedAvailableOptions((enum QueryOptions)0),
          _haveCachedAvailableOptions(false) {}

    /** helper function.  run a simple command where the command expression is simply
          { command : 1 }
        @param info -- where to put result object.  may be null if caller doesn't need that info
        @param command -- command name
        @return true if the command returned "ok".
     */
    bool simpleCommand(const std::string& dbname, BSONObj* info, const std::string& command);

    /** Run a database command.  Database commands are represented as BSON objects.  Common database
        commands have prebuilt helper functions -- see below.  If a helper is not available you can
        directly call runCommand.

        @param dbname database name.  Use "admin" for global administrative commands.
        @param cmd  the command object to execute.  For example, { ismaster : 1 }
        @param info the result object the database returns. Typically has { ok : ..., errmsg : ... }
            fields set.
        @param options see enum QueryOptions - normally not needed to run a command
        @param auth if set, the BSONObj representation will be appended to the command object sent

        @return true if the command returned "ok".
    */
    virtual bool runCommand(const std::string& dbname,
                            const BSONObj& cmd,
                            BSONObj& info,
                            int options = 0);

    /**
     * Authenticate a user.
     *
     * The "params" BSONObj should be initialized with some of the fields below.  Which fields
     * are required depends on the mechanism, which is mandatory.
     *
     *     "mechanism": The std::string name of the sasl mechanism to use.  Mandatory.
     *     "user": The std::string name of the user to authenticate.  Mandatory.
     *     "db": The database target of the auth command, which identifies the location
     *         of the credential information for the user.  May be "$external" if
     *         credential information is stored outside of the mongo cluster.  Mandatory.
     *     "pwd": The password data.
     *     "digestPassword": Boolean, set to true if the "pwd" is undigested (default).
     *     "serviceName": The GSSAPI service name to use.  Defaults to "mongodb".
     *     "serviceHostname": The GSSAPI hostname to use.  Defaults to the name of the remote
     *          host.
     *
     * Other fields in "params" are silently ignored.
     *
     * Returns normally on success, and throws on error.  Throws a DBException with getCode() ==
     * ErrorCodes::AuthenticationFailed if authentication is rejected.  All other exceptions are
     * tantamount to authentication failure, but may also indicate more serious problems.
     */
    void auth(const BSONObj& params);

    /** Authorize access to a particular database.
        Authentication is separate for each database on the server -- you may authenticate for any
        number of databases on a single connection.
        The "admin" database is special and once authenticated provides access to all databases on
        the server.
        @param      digestPassword  if password is plain text, set this to true.  otherwise assumed
                    to be pre-digested
        @param[out] authLevel       level of authentication for the given user
        @return true if successful
    */
    bool auth(const std::string& dbname,
              const std::string& username,
              const std::string& pwd,
              std::string& errmsg,
              bool digestPassword = true);

    /**
     * Logs out the connection for the given database.
     *
     * @param dbname the database to logout from.
     * @param info the result object for the logout command (provided for backwards
     *     compatibility with mongo shell)
     */
    virtual void logout(const std::string& dbname, BSONObj& info);

    /** count number of objects in collection ns that match the query criteria specified
        throws UserAssertion if database returns an error
    */
    virtual unsigned long long count(const std::string& ns,
                                     const BSONObj& query = BSONObj(),
                                     int options = 0,
                                     int limit = 0,
                                     int skip = 0);

    static std::string createPasswordDigest(const std::string& username,
                                            const std::string& clearTextPassword);

    /** returns true in isMaster parm if this db is the current master
       of a replica pair.

       pass in info for more details e.g.:
         { "ismaster" : 1.0 , "msg" : "not paired" , "ok" : 1.0  }

       returns true if command invoked successfully.
    */
    virtual bool isMaster(bool& isMaster, BSONObj* info = 0);

    /**
       Create a new collection in the database.  Normally, collection creation is automatic.  You
       would use this function if you wish to specify special options on creation.

       If the collection already exists, no action occurs.

       @param ns     fully qualified collection name
       @param size   desired initial extent size for the collection.
                     Must be <= 1000000000 for normal collections.
                     For fixed size (capped) collections, this size is the total/max size of the
                     collection.
       @param capped if true, this is a fixed size collection (where old data rolls out).
       @param max    maximum number of objects if capped (optional).

       returns true if successful.
    */
    bool createCollection(const std::string& ns,
                          long long size = 0,
                          bool capped = false,
                          int max = 0,
                          BSONObj* info = 0);

    /** Get error result from the last write operation (insert/update/delete) on this connection.
        db doesn't change the command's behavior - it is just for auth checks.
        @return error message text, or empty std::string if no error.
    */
    std::string getLastError(
        const std::string& db, bool fsync = false, bool j = false, int w = 0, int wtimeout = 0);
    /**
     * Same as the form of getLastError that takes a dbname, but just uses the admin DB.
     */
    std::string getLastError(bool fsync = false, bool j = false, int w = 0, int wtimeout = 0);

    /** Get error result from the last write operation (insert/update/delete) on this connection.
        db doesn't change the command's behavior - it is just for auth checks.
        @return full error object.

        If "w" is -1, wait for propagation to majority of nodes.
        If "wtimeout" is 0, the operation will block indefinitely if needed.
    */
    virtual BSONObj getLastErrorDetailed(
        const std::string& db, bool fsync = false, bool j = false, int w = 0, int wtimeout = 0);
    /**
     * Same as the form of getLastErrorDetailed that takes a dbname, but just uses the admin DB.
     */
    virtual BSONObj getLastErrorDetailed(bool fsync = false,
                                         bool j = false,
                                         int w = 0,
                                         int wtimeout = 0);

    /** Can be called with the returned value from getLastErrorDetailed to extract an error string.
        If all you need is the string, just call getLastError() instead.
    */
    static std::string getLastErrorString(const BSONObj& res);

    /** Return the last error which has occurred, even if not the very last operation.

       @return { err : <error message>, nPrev : <how_many_ops_back_occurred>, ok : 1 }

       result.err will be null if no error has occurred.
    */
    BSONObj getPrevError();

    /** Reset the previous error state for this connection (accessed via getLastError and
        getPrevError).  Useful when performing several operations at once and then checking
        for an error after attempting all operations.
    */
    bool resetError() {
        return simpleCommand("admin", 0, "reseterror");
    }

    /** Delete the specified collection.
     *  @param info An optional output parameter that receives the result object the database
     *  returns from the drop command.  May be null if the caller doesn't need that info.
     */
    virtual bool dropCollection(const std::string& ns, BSONObj* info = NULL) {
        std::string db = nsGetDB(ns);
        std::string coll = nsGetCollection(ns);
        uassert(10011, "no collection name", coll.size());

        BSONObj temp;
        if (info == NULL) {
            info = &temp;
        }

        bool res = runCommand(db.c_str(), BSON("drop" << coll), *info);
        resetIndexCache();
        return res;
    }

    /** Perform a repair and compaction of the specified database.  May take a long time to run.
     * Disk space must be available equal to the size of the database while repairing.
    */
    bool repairDatabase(const std::string& dbname, BSONObj* info = 0) {
        return simpleCommand(dbname, info, "repairDatabase");
    }

    /** Copy database from one server or name to another server or name.

       Generally, you should dropDatabase() first as otherwise the copied information will MERGE
       into whatever data is already present in this database.

       For security reasons this function only works when you are authorized to access the "admin"
       db.  However, if you have access to said db, you can copy any database from one place to
       another.
       TODO: this needs enhancement to be more flexible in terms of security.

       This method provides a way to "rename" a database by copying it to a new db name and
       location.  The copy is "repaired" and compacted.

       fromdb   database name from which to copy.
       todb     database name to copy to.
       fromhost hostname of the database (and optionally, ":port") from which to
                copy the data.  copies from self if "".

       returns true if successful
    */
    bool copyDatabase(const std::string& fromdb,
                      const std::string& todb,
                      const std::string& fromhost = "",
                      BSONObj* info = 0);

    /** The Mongo database provides built-in performance profiling capabilities.  Uset
     * setDbProfilingLevel() to enable.  Profiling information is then written to the system.profile
     * collection, which one can then query.
    */
    enum ProfilingLevel {
        ProfileOff = 0,
        ProfileSlow = 1,  // log very slow (>100ms) operations
        ProfileAll = 2

    };
    bool setDbProfilingLevel(const std::string& dbname, ProfilingLevel level, BSONObj* info = 0);
    bool getDbProfilingLevel(const std::string& dbname, ProfilingLevel& level, BSONObj* info = 0);


    /** This implicitly converts from char*, string, and BSONObj to be an argument to mapreduce
        You shouldn't need to explicitly construct this
     */
    struct MROutput {
        MROutput(const char* collection) : out(BSON("replace" << collection)) {}
        MROutput(const std::string& collection) : out(BSON("replace" << collection)) {}
        MROutput(const BSONObj& obj) : out(obj) {}

        BSONObj out;
    };
    static MROutput MRInline;

    /** Run a map/reduce job on the server.

        See http://dochub.mongodb.org/core/mapreduce

        ns        namespace (db+collection name) of input data
        jsmapf    javascript map function code
        jsreducef javascript reduce function code.
        query     optional query filter for the input
        output    either a std::string collection name or an object representing output type
                  if not specified uses inline output type

        returns a result object which contains:
         { result : <collection_name>,
           numObjects : <number_of_objects_scanned>,
           timeMillis : <job_time>,
           ok : <1_if_ok>,
           [, err : <errmsg_if_error>]
         }

         For example one might call:
           result.getField("ok").trueValue()
         on the result to check if ok.
    */
    BSONObj mapreduce(const std::string& ns,
                      const std::string& jsmapf,
                      const std::string& jsreducef,
                      BSONObj query = BSONObj(),
                      MROutput output = MRInline);

    /** Run javascript code on the database server.
       dbname    database SavedContext in which the code runs. The javascript variable 'db' will be
                   assigned to this database when the function is invoked.
       jscode    source code for a javascript function.
       info      the command object which contains any information on the invocation result
                   including the return value and other information.  If an error occurs running the
                   jscode, error information will be in info.  (try "log() << info.toString()")
       retValue  return value from the jscode function.
       args      args to pass to the jscode function.  when invoked, the 'args' variable will be
                   defined for use by the jscode.

       returns true if runs ok.

       See testDbEval() in dbclient.cpp for an example of usage.
    */
    bool eval(const std::string& dbname,
              const std::string& jscode,
              BSONObj& info,
              BSONElement& retValue,
              BSONObj* args = 0);

    /** validate a collection, checking for errors and reporting back statistics.
        this operation is slow and blocking.
     */
    bool validate(const std::string& ns, bool scandata = true) {
        BSONObj cmd = BSON("validate" << nsGetCollection(ns) << "scandata" << scandata);
        BSONObj info;
        return runCommand(nsGetDB(ns).c_str(), cmd, info);
    }

    /* The following helpers are simply more convenient forms of eval() for certain common cases */

    /* invocation with no return value of interest -- with or without one simple parameter */
    bool eval(const std::string& dbname, const std::string& jscode);
    template <class T>
    bool eval(const std::string& dbname, const std::string& jscode, T parm1) {
        BSONObj info;
        BSONElement retValue;
        BSONObjBuilder b;
        b.append("0", parm1);
        BSONObj args = b.done();
        return eval(dbname, jscode, info, retValue, &args);
    }

    /** eval invocation with one parm to server and one numeric field (either int or double)
     * returned */
    template <class T, class NumType>
    bool eval(const std::string& dbname, const std::string& jscode, T parm1, NumType& ret) {
        BSONObj info;
        BSONElement retValue;
        BSONObjBuilder b;
        b.append("0", parm1);
        BSONObj args = b.done();
        if (!eval(dbname, jscode, info, retValue, &args))
            return false;
        ret = (NumType)retValue.number();
        return true;
    }

    /**
       get a list of all the current databases
       uses the { listDatabases : 1 } command.
       throws on error
     */
    std::list<std::string> getDatabaseNames();

    /**
     * Get a list of all the current collections in db.
     * Returns fully qualified names.
     */
    std::list<std::string> getCollectionNames(const std::string& db);

    /**
     * { name : "<short collection name>",
     *   options : { }
     * }
     */
    std::list<BSONObj> getCollectionInfos(const std::string& db, const BSONObj& filter = BSONObj());

    bool exists(const std::string& ns);

    /** Create an index if it does not already exist.
        ensureIndex calls are remembered so it is safe/fast to call this function many
        times in your code.
       @param ns collection to be indexed
       @param keys the "key pattern" for the index.  e.g., { name : 1 }
       @param unique if true, indicates that key uniqueness should be enforced for this index
       @param name if not specified, it will be created from the keys automatically (which is
            recommended)
       @param cache if set to false, the index cache for the connection won't remember this call
       @param background build index in the background (see mongodb docs for details)
       @param v index version. leave at default value. (unit tests set this parameter.)
       @param ttl. The value of how many seconds before data should be removed from a collection.
       @return whether or not sent message to db.
         should be true on first call, false on subsequent unless resetIndexCache was called
     */
    virtual bool ensureIndex(const std::string& ns,
                             BSONObj keys,
                             bool unique = false,
                             const std::string& name = "",
                             bool cache = true,
                             bool background = false,
                             int v = -1,
                             int ttl = 0);
    /**
       clears the index cache, so the subsequent call to ensureIndex for any index will go to the
       server
     */
    virtual void resetIndexCache();

    virtual std::list<BSONObj> getIndexSpecs(const std::string& ns, int options = 0);

    virtual void dropIndex(const std::string& ns, BSONObj keys);
    virtual void dropIndex(const std::string& ns, const std::string& indexName);

    /**
       drops all indexes for the collection
     */
    virtual void dropIndexes(const std::string& ns);

    virtual void reIndex(const std::string& ns);

    static std::string genIndexName(const BSONObj& keys);

    /** Erase / drop an entire database */
    virtual bool dropDatabase(const std::string& dbname, BSONObj* info = 0) {
        bool ret = simpleCommand(dbname, info, "dropDatabase");
        resetIndexCache();
        return ret;
    }

    virtual std::string toString() const = 0;

    /**
     * A function type for runCommand hooking; the function takes a pointer
     * to a BSONObjBuilder and returns nothing.  The builder contains a
     * runCommand BSON object.
     * Once such a function is set as the runCommand hook, every time the DBClient
     * processes a runCommand, the hook will be called just prior to sending it to the server.
     */
    typedef stdx::function<void(BSONObjBuilder*)> RunCommandHookFunc;
    virtual void setRunCommandHook(RunCommandHookFunc func);
    RunCommandHookFunc getRunCommandHook() const {
        return _runCommandHook;
    }

    /**
     * Similar to above, but for running a function on a command response after a command
     * has been run.
     */
    typedef stdx::function<void(const BSONObj&, const std::string&)> PostRunCommandHookFunc;
    virtual void setPostRunCommandHook(PostRunCommandHookFunc func);
    PostRunCommandHookFunc getPostRunCommandHook() const {
        return _postRunCommandHook;
    }


protected:
    /** if the result of a command is ok*/
    bool isOk(const BSONObj&);

    /** if the element contains a not master error */
    bool isNotMasterErrorString(const BSONElement& e);

    BSONObj _countCmd(
        const std::string& ns, const BSONObj& query, int options, int limit, int skip);

    /**
     * Look up the options available on this client.  Caches the answer from
     * _lookupAvailableOptions(), below.
     */
    QueryOptions availableOptions();

    virtual QueryOptions _lookupAvailableOptions();

    virtual void _auth(const BSONObj& params);

    /**
     * Use the MONGODB-CR protocol to authenticate as "username" against the database "dbname",
     * with the given password.  If digestPassword is false, the password is assumed to be
     * pre-digested.  Returns false on failure, and sets "errmsg".
     */
    bool _authMongoCR(const std::string& dbname,
                      const std::string& username,
                      const std::string& pwd,
                      BSONObj* info,
                      bool digestPassword);

    /**
     * Use the MONGODB-X509 protocol to authenticate as "username. The certificate details
     * has already been communicated automatically as part of the connect call.
     * Returns false on failure and set "errmsg".
     */
    bool _authX509(const std::string& dbname, const std::string& username, BSONObj* info);

    /**
     * These functions will be executed by the driver on runCommand calls.
     */
    RunCommandHookFunc _runCommandHook;
    PostRunCommandHookFunc _postRunCommandHook;


private:
    enum QueryOptions _cachedAvailableOptions;
    bool _haveCachedAvailableOptions;
};

/**
 abstract class that implements the core db operations
 */
class MONGO_CLIENT_API DBClientBase : public DBClientWithCommands, public DBConnector {
protected:
    static AtomicInt64 ConnectionIdSequence;
    long long _connectionId;  // unique connection id for this connection
    WriteConcern _writeConcern;
    int _minWireVersion;
    int _maxWireVersion;

public:
    static const uint64_t INVALID_SOCK_CREATION_TIME;

    DBClientBase() {
        _writeConcern = W_NORMAL;
        _connectionId = ConnectionIdSequence.fetchAndAdd(1);
        _minWireVersion = _maxWireVersion = 0;
    }

    long long getConnectionId() const {
        return _connectionId;
    }

    WriteConcern getWriteConcern() const {
        return _writeConcern;
    }
    void setWriteConcern(WriteConcern w) {
        _writeConcern = w;
    }

    void setWireVersions(int minWireVersion, int maxWireVersion) {
        _minWireVersion = minWireVersion;
        _maxWireVersion = maxWireVersion;
    }

    int getMinWireVersion() {
        return _minWireVersion;
    }
    int getMaxWireVersion() {
        return _maxWireVersion;
    }

    /** send a query to the database.
     @param ns namespace to query, format is <dbname>.<collectname>[.<collectname>]*
     @param query query to perform on the collection.  this is a BSONObj (binary JSON)
     You may format as
       { query: { ... }, orderby: { ... } }
     to specify a sort order.
     @param nToReturn n to return (i.e., limit).  0 = unlimited
     @param nToSkip start with the nth item
     @param fieldsToReturn optional template of which fields to select. if unspecified, returns all
            fields
     @param queryOptions see options enum at top of this file

     @return    cursor.   0 if error (connection failure)
     @throws AssertionException
    */
    virtual std::auto_ptr<DBClientCursor> query(const std::string& ns,
                                                Query query,
                                                int nToReturn = 0,
                                                int nToSkip = 0,
                                                const BSONObj* fieldsToReturn = 0,
                                                int queryOptions = 0,
                                                int batchSize = 0);


    /** Uses QueryOption_Exhaust, when available.

        Exhaust mode sends back all data queries as fast as possible, with no back-and-forth for
        OP_GETMORE.  If you are certain you will exhaust the query, it could be useful.

        Use the DBClientCursorBatchIterator version, below, if you want to do items in large
        blocks, perhaps to avoid granular locking and such.
     */
    virtual unsigned long long query(stdx::function<void(const BSONObj&)> f,
                                     const std::string& ns,
                                     Query query,
                                     const BSONObj* fieldsToReturn = 0,
                                     int queryOptions = 0);

    virtual unsigned long long query(stdx::function<void(DBClientCursorBatchIterator&)> f,
                                     const std::string& ns,
                                     Query query,
                                     const BSONObj* fieldsToReturn = 0,
                                     int queryOptions = 0);


    /** don't use this - called automatically by DBClientCursor for you
        @param cursorId id of cursor to retrieve
        @return an handle to a previously allocated cursor
        @throws AssertionException
     */
    virtual std::auto_ptr<DBClientCursor> getMore(const std::string& ns,
                                                  long long cursorId,
                                                  int nToReturn = 0,
                                                  int options = 0);

    /**
       insert an object into the database
     */
    virtual void insert(const std::string& ns, BSONObj obj, int flags = 0);

    /**
       insert a vector of objects into the database
     */
    virtual void insert(const std::string& ns, const std::vector<BSONObj>& v, int flags = 0);

    /**
       updates objects matching query
     */
    virtual void update(
        const std::string& ns, Query query, BSONObj obj, bool upsert = false, bool multi = false);

    virtual void update(const std::string& ns, Query query, BSONObj obj, int flags);

    /**
       remove matching objects from the database
       @param justOne if this true, then once a single match is found will stop
     */
    virtual void remove(const std::string& ns, Query q, bool justOne = 0);

    virtual void remove(const std::string& ns, Query query, int flags);

    virtual bool isFailed() const = 0;

    /**
     * if not checked recently, checks whether the underlying socket/sockets are still valid
     */
    virtual bool isStillConnected() = 0;

    virtual void killCursor(long long cursorID) = 0;

    virtual bool callRead(Message& toSend, Message& response) = 0;
    // virtual bool callWrite( Message& toSend , Message& response ) = 0; //TODO: add this if needed

    virtual ConnectionString::ConnectionType type() const = 0;

    virtual double getSoTimeout() const = 0;

    virtual uint64_t getSockCreationMicroSec() const {
        return INVALID_SOCK_CREATION_TIME;
    }

    virtual void reset() {}

};  // DBClientBase

class DBClientReplicaSet;

class MONGO_CLIENT_API ConnectException : public UserException {
public:
    ConnectException(std::string msg) : UserException(9000, msg) {}
};

/**
    A basic connection to the database.
    This is the main entry point for talking to a simple Mongo setup
*/
class MONGO_CLIENT_API DBClientConnection : public DBClientBase {
public:
    using DBClientBase::query;

    /**
       @param _autoReconnect if true, automatically reconnect on a connection failure
       @param timeout tcp timeout in seconds - this is for read/write, not connect.
       Connect timeout is fixed, but short, at 5 seconds.
     */
    DBClientConnection(bool _autoReconnect = false, double so_timeout = 0);

    virtual ~DBClientConnection() {
        _numConnections.fetchAndAdd(-1);
    }

    /** Connect to a Mongo database server.

       If autoReconnect is true, you can try to use the DBClientConnection even when
       false was returned -- it will try to connect again.

       @param server server to connect to.
       @param errmsg any relevant error message will appended to the string
       @return false if fails to connect.
    */
    virtual bool connect(const HostAndPort& server, std::string& errmsg);

    /** Connect to a Mongo database server.  Exception throwing version.
        Throws a UserException if cannot connect.

       If autoReconnect is true, you can try to use the DBClientConnection even when
       false was returned -- it will try to connect again.

       @param serverHostname host to connect to.  can include port number ( 127.0.0.1 ,
        127.0.0.1:5555 )
    */
    void connect(const std::string& serverHostname) {
        std::string errmsg;
        if (!connect(HostAndPort(serverHostname), errmsg))
            throw ConnectException(std::string("can't connect ") + errmsg);
    }

    /**
     * Logs out the connection for the given database.
     *
     * @param dbname the database to logout from.
     * @param info the result object for the logout command (provided for backwards
     *     compatibility with mongo shell)
     */
    virtual void logout(const std::string& dbname, BSONObj& info);

    virtual std::auto_ptr<DBClientCursor> query(const std::string& ns,
                                                Query query = Query(),
                                                int nToReturn = 0,
                                                int nToSkip = 0,
                                                const BSONObj* fieldsToReturn = 0,
                                                int queryOptions = 0,
                                                int batchSize = 0) {
        checkConnection();
        return DBClientBase::query(
            ns, query, nToReturn, nToSkip, fieldsToReturn, queryOptions, batchSize);
    }

    virtual unsigned long long query(stdx::function<void(DBClientCursorBatchIterator&)> f,
                                     const std::string& ns,
                                     Query query,
                                     const BSONObj* fieldsToReturn,
                                     int queryOptions);

    virtual bool runCommand(const std::string& dbname,
                            const BSONObj& cmd,
                            BSONObj& info,
                            int options = 0);

    /**
       @return true if this connection is currently in a failed state.  When autoreconnect is on,
               a connection will transition back to an ok state after reconnecting.
     */
    bool isFailed() const {
        return _failed;
    }

    bool isStillConnected() {
        return p ? p->isStillConnected() : true;
    }

    MessagingPort& port() {
        verify(p);
        return *p;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << _serverString;
        if (!_serverAddrString.empty())
            ss << " (" << _serverAddrString << ")";
        if (_failed)
            ss << " failed";
        return ss.str();
    }

    std::string getServerAddress() const {
        return _serverString;
    }
    const HostAndPort& getServerHostAndPort() const {
        return _server;
    }

    virtual void killCursor(long long cursorID);
    virtual bool callRead(Message& toSend, Message& response) {
        return call(toSend, response);
    }
    virtual void say(Message& toSend, bool isRetry = false, std::string* actualServer = 0);
    virtual bool recv(Message& m);
    virtual void checkResponse(const char* data,
                               int nReturned,
                               bool* retry = NULL,
                               std::string* host = NULL);
    virtual bool call(Message& toSend,
                      Message& response,
                      bool assertOk = true,
                      std::string* actualServer = 0);
    virtual ConnectionString::ConnectionType type() const {
        return ConnectionString::MASTER;
    }
    void setSoTimeout(double timeout);
    double getSoTimeout() const {
        return _so_timeout;
    }

    virtual bool lazySupported() const {
        return true;
    }

    static int getNumConnections() {
        return _numConnections.load();
    }

    /**
     * Set the name of the replica set that this connection is associated to.
     * Note: There is no validation on replSetName.
     */
    void setParentReplSetName(const std::string& replSetName);

    static void setLazyKillCursor(bool lazy) {
        _lazyKillCursor = lazy;
    }
    static bool getLazyKillCursor() {
        return _lazyKillCursor;
    }

    uint64_t getSockCreationMicroSec() const;

protected:
    friend class SyncClusterConnection;
    virtual void _auth(const BSONObj& params);
    virtual void sayPiggyBack(Message& toSend);

    boost::scoped_ptr<MessagingPort> p;
    boost::scoped_ptr<SockAddr> server;
    bool _failed;
    const bool autoReconnect;
    Backoff autoReconnectBackoff;
    HostAndPort _server;            // remember for reconnects
    std::string _serverString;      // server host and port
    std::string _serverAddrString;  // resolved ip of server
    void _checkConnection();

    // throws SocketException if in failed state and not reconnecting or if waiting to reconnect
    void checkConnection() {
        if (_failed)
            _checkConnection();
    }

    std::map<std::string, BSONObj> authCache;
    double _so_timeout;
    bool _connect(std::string& errmsg);

    static AtomicInt32 _numConnections;
    static bool _lazyKillCursor;  // lazy means we piggy back kill cursors on next op

#ifdef MONGO_SSL
    SSLManagerInterface* sslManager();
#endif

private:
    /**
     * Checks the BSONElement for the 'not master' keyword and if it does exist,
     * try to inform the replica set monitor that the host this connects to is
     * no longer primary.
     */
    void handleNotMasterResponse(const BSONElement& elemToCheck);

    // Contains the string for the replica set name of the host this is connected to.
    // Should be empty if this connection is not pointing to a replica set member.
    std::string _parentReplSetName;
};

/** pings server to check if it's up
 */
MONGO_CLIENT_API bool serverAlive(const std::string& uri);

MONGO_CLIENT_API BSONElement getErrField(const BSONObj& result);
MONGO_CLIENT_API bool hasErrField(const BSONObj& result);

MONGO_CLIENT_API inline std::ostream& operator<<(std::ostream& s, const Query& q) {
    return s << q.toString();
}

}  // namespace mongo

#include "mongo/client/dbclientcursor.h"
