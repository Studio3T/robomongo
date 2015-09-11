// dbclient.cpp - connect to a Mongo database as a database, from C++

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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kNetwork

#include "mongo/platform/basic.h"

#include "mongo/bson/util/bson_extract.h"
#include "mongo/bson/util/builder.h"
#include "mongo/client/constants.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/client/dbclient_rs.h"
#include "mongo/client/replica_set_monitor.h"
#include "mongo/client/sasl_client_authenticate.h"
#include "mongo/client/syncclusterconnection.h"
#include "mongo/db/auth/internal_user_auth.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/json.h"
#include "mongo/db/namespace_string.h"
#include "mongo/s/stale_exception.h"  // for RecvStaleConfigException
#include "mongo/util/assert_util.h"
#include "mongo/util/log.h"
#include "mongo/util/net/ssl_manager.h"
#include "mongo/util/net/ssl_options.h"
#include "mongo/util/password_digest.h"

namespace mongo {

using std::auto_ptr;
using std::endl;
using std::list;
using std::map;
using std::string;
using std::stringstream;
using std::vector;

AtomicInt64 DBClientBase::ConnectionIdSequence;

const char* const saslCommandUserSourceFieldName = "userSource";

void ConnectionString::_fillServers(string s) {
    //
    // Custom-handled servers/replica sets start with '$'
    // According to RFC-1123/952, this will not overlap with valid hostnames
    // (also disallows $replicaSetName hosts)
    //

    if (s.find('$') == 0)
        _type = CUSTOM;

    {
        string::size_type idx = s.find('/');
        if (idx != string::npos) {
            _setName = s.substr(0, idx);
            s = s.substr(idx + 1);
            if (_type != CUSTOM)
                _type = SET;
        }
    }

    string::size_type idx;
    while ((idx = s.find(',')) != string::npos) {
        _servers.push_back(HostAndPort(s.substr(0, idx)));
        s = s.substr(idx + 1);
    }
    _servers.push_back(HostAndPort(s));
}

void ConnectionString::_finishInit() {
    // Needed here as well b/c the parsing logic isn't used in all constructors
    // TODO: Refactor so that the parsing logic *is* used in all constructors
    if (_type == MASTER && _servers.size() > 0) {
        if (_servers[0].host().find('$') == 0) {
            _type = CUSTOM;
        }
    }

    stringstream ss;
    if (_type == SET)
        ss << _setName << "/";
    for (unsigned i = 0; i < _servers.size(); i++) {
        if (i > 0)
            ss << ",";
        ss << _servers[i].toString();
    }
    _string = ss.str();
}

mutex ConnectionString::_connectHookMutex("ConnectionString::_connectHook");
ConnectionString::ConnectionHook* ConnectionString::_connectHook = NULL;

DBClientBase* ConnectionString::connect(string& errmsg, double socketTimeout) const {
    switch (_type) {
        case MASTER: {
            auto_ptr<DBClientConnection> c(new DBClientConnection(true));
            c->setSoTimeout(socketTimeout);
            LOG(1) << "creating new connection to:" << _servers[0] << endl;
            if (!c->connect(_servers[0], errmsg)) {
                return 0;
            }
            LOG(1) << "connected connection!" << endl;
            return c.release();
        }

        case PAIR:
        case SET: {
            auto_ptr<DBClientReplicaSet> set(
                new DBClientReplicaSet(_setName, _servers, socketTimeout));

            if (!set->connect()) {
                errmsg = "connect failed to replica set ";
                errmsg += toString();
                return 0;
            }
            return set.release();
        }

        case SYNC: {
            // TODO , don't copy
            list<HostAndPort> l;
            for (unsigned i = 0; i < _servers.size(); i++)
                l.push_back(_servers[i]);
            SyncClusterConnection* c = new SyncClusterConnection(l, socketTimeout);
            return c;
        }

        case CUSTOM: {
            // Lock in case other things are modifying this at the same time
            scoped_lock lk(_connectHookMutex);

            // Allow the replacement of connections with other connections - useful for testing.

            uassert(16335,
                    "custom connection to " + this->toString() +
                        " specified with no connection hook",
                    _connectHook);

            // Double-checked lock, since this will never be active during normal operation
            DBClientBase* replacementConn = _connectHook->connect(*this, errmsg, socketTimeout);

            log() << "replacing connection to " << this->toString() << " with "
                  << (replacementConn ? replacementConn->getServerAddress() : "(empty)") << endl;

            return replacementConn;
        }

        case INVALID:
            throw UserException(13421, "trying to connect to invalid ConnectionString");
            break;
    }

    verify(0);
    return 0;
}

bool ConnectionString::sameLogicalEndpoint(const ConnectionString& other) const {
    if (_type != other._type)
        return false;

    switch (_type) {
        case INVALID:
            return true;
        case MASTER:
            return _servers[0] == other._servers[0];
        case PAIR:
            if (_servers[0] == other._servers[0])
                return _servers[1] == other._servers[1];
            return (_servers[0] == other._servers[1]) && (_servers[1] == other._servers[0]);
        case SET:
            return _setName == other._setName;
        case SYNC:
            // The servers all have to be the same in each, but not in the same order.
            if (_servers.size() != other._servers.size())
                return false;
            for (unsigned i = 0; i < _servers.size(); i++) {
                bool found = false;
                for (unsigned j = 0; j < other._servers.size(); j++) {
                    if (_servers[i] == other._servers[j]) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    return false;
            }
            return true;
        case CUSTOM:
            return _string == other._string;
    }
    verify(false);
}

ConnectionString ConnectionString::parse(const string& host, string& errmsg) {
    string::size_type i = host.find('/');
    if (i != string::npos && i != 0) {
        // replica set
        return ConnectionString(SET, host.substr(i + 1), host.substr(0, i));
    }

    int numCommas = str::count(host, ',');

    if (numCommas == 0)
        return ConnectionString(HostAndPort(host));

    if (numCommas == 1)
        return ConnectionString(PAIR, host);

    if (numCommas == 2)
        return ConnectionString(SYNC, host);

    errmsg = (string) "invalid hostname [" + host + "]";
    return ConnectionString();  // INVALID
}

string ConnectionString::typeToString(ConnectionType type) {
    switch (type) {
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

const BSONField<BSONObj> Query::ReadPrefField("$readPreference");
const BSONField<string> Query::ReadPrefModeField("mode");
const BSONField<BSONArray> Query::ReadPrefTagsField("tags");

Query::Query(const string& json) : obj(fromjson(json)) {}

Query::Query(const char* json) : obj(fromjson(json)) {}

Query& Query::hint(const string& jsonKeyPatt) {
    return hint(fromjson(jsonKeyPatt));
}

Query& Query::where(const string& jscode, BSONObj scope) {
    /* use where() before sort() and hint() and explain(), else this will assert. */
    verify(!isComplex());
    BSONObjBuilder b;
    b.appendElements(obj);
    b.appendWhere(jscode, scope);
    obj = b.obj();
    return *this;
}

void Query::makeComplex() {
    if (isComplex())
        return;
    BSONObjBuilder b;
    b.append("query", obj);
    obj = b.obj();
}

Query& Query::sort(const BSONObj& s) {
    appendComplex("orderby", s);
    return *this;
}

Query& Query::hint(BSONObj keyPattern) {
    appendComplex("$hint", keyPattern);
    return *this;
}

Query& Query::explain() {
    appendComplex("$explain", true);
    return *this;
}

Query& Query::snapshot() {
    appendComplex("$snapshot", true);
    return *this;
}

Query& Query::minKey(const BSONObj& val) {
    appendComplex("$min", val);
    return *this;
}

Query& Query::maxKey(const BSONObj& val) {
    appendComplex("$max", val);
    return *this;
}

bool Query::isComplex(const BSONObj& obj, bool* hasDollar) {
    if (obj.hasElement("query")) {
        if (hasDollar)
            *hasDollar = false;
        return true;
    }

    if (obj.hasElement("$query")) {
        if (hasDollar)
            *hasDollar = true;
        return true;
    }

    return false;
}

Query& Query::readPref(ReadPreference pref, const BSONArray& tags) {
    string mode;

    switch (pref) {
        case ReadPreference_PrimaryOnly:
            mode = "primary";
            break;

        case ReadPreference_PrimaryPreferred:
            mode = "primaryPreferred";
            break;

        case ReadPreference_SecondaryOnly:
            mode = "secondary";
            break;

        case ReadPreference_SecondaryPreferred:
            mode = "secondaryPreferred";
            break;

        case ReadPreference_Nearest:
            mode = "nearest";
            break;
    }

    BSONObjBuilder readPrefDocBuilder;
    readPrefDocBuilder << ReadPrefModeField(mode);

    if (!tags.isEmpty()) {
        readPrefDocBuilder << ReadPrefTagsField(tags);
    }

    appendComplex(ReadPrefField.name().c_str(), readPrefDocBuilder.done());
    return *this;
}

bool Query::isComplex(bool* hasDollar) const {
    return isComplex(obj, hasDollar);
}

bool Query::hasReadPreference(const BSONObj& queryObj) {
    const bool hasReadPrefOption = queryObj["$queryOptions"].isABSONObj() &&
        queryObj["$queryOptions"].Obj().hasField(ReadPrefField.name());
    return (Query::isComplex(queryObj) && queryObj.hasField(ReadPrefField.name())) ||
        hasReadPrefOption;
}

BSONObj Query::getFilter() const {
    bool hasDollar;
    if (!isComplex(&hasDollar))
        return obj;

    return obj.getObjectField(hasDollar ? "$query" : "query");
}
BSONObj Query::getSort() const {
    if (!isComplex())
        return BSONObj();
    BSONObj ret = obj.getObjectField("orderby");
    if (ret.isEmpty())
        ret = obj.getObjectField("$orderby");
    return ret;
}
BSONObj Query::getHint() const {
    if (!isComplex())
        return BSONObj();
    return obj.getObjectField("$hint");
}
bool Query::isExplain() const {
    return isComplex() && obj.getBoolField("$explain");
}

string Query::toString() const {
    return obj.toString();
}

/* --- dbclientcommands --- */

bool DBClientWithCommands::isOk(const BSONObj& o) {
    return o["ok"].trueValue();
}

bool DBClientWithCommands::isNotMasterErrorString(const BSONElement& e) {
    return e.type() == String && str::contains(e.valuestr(), "not master");
}


enum QueryOptions DBClientWithCommands::availableOptions() {
    if (!_haveCachedAvailableOptions) {
        _cachedAvailableOptions = _lookupAvailableOptions();
        _haveCachedAvailableOptions = true;
    }
    return _cachedAvailableOptions;
}

enum QueryOptions DBClientWithCommands::_lookupAvailableOptions() {
    BSONObj ret;
    if (runCommand("admin", BSON("availablequeryoptions" << 1), ret)) {
        return QueryOptions(ret.getIntField("options"));
    }
    return QueryOptions(0);
}

void DBClientWithCommands::setRunCommandHook(RunCommandHookFunc func) {
    _runCommandHook = func;
}

void DBClientWithCommands::setPostRunCommandHook(PostRunCommandHookFunc func) {
    _postRunCommandHook = func;
}

bool DBClientWithCommands::runCommand(const string& dbname,
                                      const BSONObj& cmd,
                                      BSONObj& info,
                                      int options) {
    string ns = dbname + ".$cmd";
    if (_runCommandHook) {
        BSONObjBuilder cmdObj;
        cmdObj.appendElements(cmd);
        _runCommandHook(&cmdObj);

        info = findOne(ns, cmdObj.done(), 0, options);
    } else {
        info = findOne(ns, cmd, 0, options);
    }
    if (_postRunCommandHook) {
        _postRunCommandHook(info, getServerAddress());
    }
    return isOk(info);
}

/* note - we build a bson obj here -- for something that is super common like getlasterror you
          should have that object prebuilt as that would be faster.
*/
bool DBClientWithCommands::simpleCommand(const string& dbname,
                                         BSONObj* info,
                                         const string& command) {
    BSONObj o;
    if (info == 0)
        info = &o;
    BSONObjBuilder b;
    b.append(command, 1);
    return runCommand(dbname, b.done(), *info);
}

unsigned long long DBClientWithCommands::count(
    const string& myns, const BSONObj& query, int options, int limit, int skip) {
    BSONObj cmd = _countCmd(myns, query, options, limit, skip);
    BSONObj res;
    if (!runCommand(nsToDatabase(myns), cmd, res, options))
        uasserted(11010, string("count fails:") + res.toString());
    return res["n"].numberLong();
}

BSONObj DBClientWithCommands::_countCmd(
    const string& myns, const BSONObj& query, int options, int limit, int skip) {
    NamespaceString ns(myns);
    BSONObjBuilder b;
    b.append("count", ns.coll());
    b.append("query", query);
    if (limit)
        b.append("limit", limit);
    if (skip)
        b.append("skip", skip);
    return b.obj();
}

BSONObj DBClientWithCommands::getLastErrorDetailed(bool fsync, bool j, int w, int wtimeout) {
    return getLastErrorDetailed("admin", fsync, j, w, wtimeout);
}

BSONObj DBClientWithCommands::getLastErrorDetailed(
    const std::string& db, bool fsync, bool j, int w, int wtimeout) {
    BSONObj info;
    BSONObjBuilder b;
    b.append("getlasterror", 1);

    if (fsync)
        b.append("fsync", 1);
    if (j)
        b.append("j", 1);

    // only affects request when greater than one node
    if (w >= 1)
        b.append("w", w);
    else if (w == -1)
        b.append("w", "majority");

    if (wtimeout > 0)
        b.append("wtimeout", wtimeout);

    runCommand(db, b.obj(), info);

    return info;
}

string DBClientWithCommands::getLastError(bool fsync, bool j, int w, int wtimeout) {
    return getLastError("admin", fsync, j, w, wtimeout);
}

string DBClientWithCommands::getLastError(
    const std::string& db, bool fsync, bool j, int w, int wtimeout) {
    BSONObj info = getLastErrorDetailed(db, fsync, j, w, wtimeout);
    return getLastErrorString(info);
}

string DBClientWithCommands::getLastErrorString(const BSONObj& info) {
    if (info["ok"].trueValue()) {
        BSONElement e = info["err"];
        if (e.eoo())
            return "";
        if (e.type() == Object)
            return e.toString();
        return e.str();
    } else {
        // command failure
        BSONElement e = info["errmsg"];
        if (e.eoo())
            return "";
        if (e.type() == Object)
            return "getLastError command failed: " + e.toString();
        return "getLastError command failed: " + e.str();
    }
}

const BSONObj getpreverrorcmdobj = fromjson("{getpreverror:1}");

BSONObj DBClientWithCommands::getPrevError() {
    BSONObj info;
    runCommand("admin", getpreverrorcmdobj, info);
    return info;
}

BSONObj getnoncecmdobj = fromjson("{getnonce:1}");

string DBClientWithCommands::createPasswordDigest(const string& username,
                                                  const string& clearTextPassword) {
    return mongo::createPasswordDigest(username, clearTextPassword);
}

namespace {
class RunCommandHookOverrideGuard {
    MONGO_DISALLOW_COPYING(RunCommandHookOverrideGuard);

public:
    RunCommandHookOverrideGuard(DBClientWithCommands* cli,
                                const DBClientWithCommands::RunCommandHookFunc& hookFunc)
        : _cli(cli), _oldHookFunc(cli->getRunCommandHook()) {
        cli->setRunCommandHook(hookFunc);
    }
    ~RunCommandHookOverrideGuard() {
        _cli->setRunCommandHook(_oldHookFunc);
    }

private:
    DBClientWithCommands* const _cli;
    DBClientWithCommands::RunCommandHookFunc const _oldHookFunc;
};
}  // namespace

void DBClientWithCommands::_auth(const BSONObj& params) {
    RunCommandHookOverrideGuard hookGuard(this, RunCommandHookFunc());
    std::string mechanism;

    uassertStatusOK(bsonExtractStringField(params, saslCommandMechanismFieldName, &mechanism));

    uassert(17232,
            "You cannot specify both 'db' and 'userSource'. Please use only 'db'.",
            !(params.hasField(saslCommandUserDBFieldName) &&
              params.hasField(saslCommandUserSourceFieldName)));

    if (mechanism == StringData("MONGODB-CR", StringData::LiteralTag())) {
        std::string db;
        if (params.hasField(saslCommandUserSourceFieldName)) {
            uassertStatusOK(bsonExtractStringField(params, saslCommandUserSourceFieldName, &db));
        } else {
            uassertStatusOK(bsonExtractStringField(params, saslCommandUserDBFieldName, &db));
        }
        std::string user;
        uassertStatusOK(bsonExtractStringField(params, saslCommandUserFieldName, &user));
        std::string password;
        uassertStatusOK(bsonExtractStringField(params, saslCommandPasswordFieldName, &password));
        bool digestPassword;
        uassertStatusOK(bsonExtractBooleanFieldWithDefault(
            params, saslCommandDigestPasswordFieldName, true, &digestPassword));
        BSONObj result;
        uassert(result["code"].Int(),
                result.toString(),
                _authMongoCR(db, user, password, &result, digestPassword));
    }
#ifdef MONGO_SSL
    else if (mechanism == StringData("MONGODB-X509", StringData::LiteralTag())) {
        std::string db;
        if (params.hasField(saslCommandUserSourceFieldName)) {
            uassertStatusOK(bsonExtractStringField(params, saslCommandUserSourceFieldName, &db));
        } else {
            uassertStatusOK(bsonExtractStringField(params, saslCommandUserDBFieldName, &db));
        }
        std::string user;
        uassertStatusOK(bsonExtractStringField(params, saslCommandUserFieldName, &user));

        uassert(ErrorCodes::AuthenticationFailed,
                "Please enable SSL on the client-side to use the MONGODB-X509 "
                "authentication mechanism.",
                getSSLManager() != NULL);

        uassert(ErrorCodes::AuthenticationFailed,
                "Username \"" + user + "\" does not match the provided client certificate user \"" +
                    getSSLManager()->getSSLConfiguration().clientSubjectName + "\"",
                user == getSSLManager()->getSSLConfiguration().clientSubjectName);

        BSONObj result;
        uassert(result["code"].Int(), result.toString(), _authX509(db, user, &result));
    }
#endif
    else if (saslClientAuthenticate != NULL) {
        uassertStatusOK(saslClientAuthenticate(this, params));
    } else {
        uasserted(ErrorCodes::BadValue,
                  mechanism + " mechanism support not compiled into client library.");
    }
};

void DBClientWithCommands::auth(const BSONObj& params) {
    try {
        _auth(params);
        return;
    } catch (const UserException& ex) {
        if (getFallbackAuthParams(params).isEmpty() ||
            (ex.getCode() != ErrorCodes::BadValue && ex.getCode() != ErrorCodes::CommandNotFound)) {
            throw ex;
        }
    }

    // BadValue or CommandNotFound indicates unsupported auth mechanism so fall back to
    // MONGODB-CR for 2.6 compatibility.
    _auth(getFallbackAuthParams(params));
}

bool DBClientWithCommands::auth(const string& dbname,
                                const string& username,
                                const string& password_text,
                                string& errmsg,
                                bool digestPassword) {
    try {
        auth(BSON(saslCommandMechanismFieldName
                  << "SCRAM-SHA-1" << saslCommandUserDBFieldName << dbname
                  << saslCommandUserFieldName << username << saslCommandPasswordFieldName
                  << password_text << saslCommandDigestPasswordFieldName << digestPassword));
        return true;
    } catch (const UserException& ex) {
        if (ex.getCode() != ErrorCodes::AuthenticationFailed)
            throw;
        errmsg = ex.what();
        return false;
    }
}

bool DBClientWithCommands::_authMongoCR(const string& dbname,
                                        const string& username,
                                        const string& password_text,
                                        BSONObj* info,
                                        bool digestPassword) {
    string password = password_text;
    if (digestPassword)
        password = createPasswordDigest(username, password_text);

    string nonce;
    if (!runCommand(dbname, getnoncecmdobj, *info)) {
        return false;
    }
    {
        BSONElement e = info->getField("nonce");
        verify(e.type() == String);
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
            md5_append(&st, (const md5_byte_t*)nonce.c_str(), nonce.size());
            md5_append(&st, (const md5_byte_t*)username.data(), username.length());
            md5_append(&st, (const md5_byte_t*)password.c_str(), password.size());
            md5_finish(&st, d);
        }
        b << "key" << digestToString(d);
        authCmd = b.done();
    }

    if (runCommand(dbname, authCmd, *info)) {
        return true;
    }

    return false;
}

bool DBClientWithCommands::_authX509(const string& dbname, const string& username, BSONObj* info) {
    BSONObj authCmd;
    BSONObjBuilder cmdBuilder;
    cmdBuilder << "authenticate" << 1 << "mechanism"
               << "MONGODB-X509"
               << "user" << username;
    authCmd = cmdBuilder.done();

    if (runCommand(dbname, authCmd, *info)) {
        return true;
    }

    return false;
}

void DBClientWithCommands::logout(const string& dbname, BSONObj& info) {
    runCommand(dbname, BSON("logout" << 1), info);
}

BSONObj ismastercmdobj = fromjson("{\"ismaster\":1}");

bool DBClientWithCommands::isMaster(bool& isMaster, BSONObj* info) {
    BSONObj o;
    if (info == 0)
        info = &o;
    bool ok = runCommand("admin", ismastercmdobj, *info);
    isMaster = info->getField("ismaster").trueValue();
    return ok;
}

bool DBClientWithCommands::createCollection(
    const string& ns, long long size, bool capped, int max, BSONObj* info) {
    verify(!capped || size);
    BSONObj o;
    if (info == 0)
        info = &o;
    BSONObjBuilder b;
    string db = nsToDatabase(ns);
    b.append("create", ns.c_str() + db.length() + 1);
    if (size)
        b.append("size", size);
    if (capped)
        b.append("capped", true);
    if (max)
        b.append("max", max);
    return runCommand(db.c_str(), b.done(), *info);
}

bool DBClientWithCommands::copyDatabase(const string& fromdb,
                                        const string& todb,
                                        const string& fromhost,
                                        BSONObj* info) {
    BSONObj o;
    if (info == 0)
        info = &o;
    BSONObjBuilder b;
    b.append("copydb", 1);
    b.append("fromhost", fromhost);
    b.append("fromdb", fromdb);
    b.append("todb", todb);
    return runCommand("admin", b.done(), *info);
}

bool DBClientWithCommands::setDbProfilingLevel(const string& dbname,
                                               ProfilingLevel level,
                                               BSONObj* info) {
    BSONObj o;
    if (info == 0)
        info = &o;

    if (level) {
        // Create system.profile collection.  If it already exists this does nothing.
        // TODO: move this into the db instead of here so that all
        //       drivers don't have to do this.
        string ns = dbname + ".system.profile";
        createCollection(ns.c_str(), 1024 * 1024, true, 0, info);
    }

    BSONObjBuilder b;
    b.append("profile", (int)level);
    return runCommand(dbname, b.done(), *info);
}

BSONObj getprofilingcmdobj = fromjson("{\"profile\":-1}");

bool DBClientWithCommands::getDbProfilingLevel(const string& dbname,
                                               ProfilingLevel& level,
                                               BSONObj* info) {
    BSONObj o;
    if (info == 0)
        info = &o;
    if (runCommand(dbname, getprofilingcmdobj, *info)) {
        level = (ProfilingLevel)info->getIntField("was");
        return true;
    }
    return false;
}

DBClientWithCommands::MROutput DBClientWithCommands::MRInline(BSON("inline" << 1));

BSONObj DBClientWithCommands::mapreduce(const string& ns,
                                        const string& jsmapf,
                                        const string& jsreducef,
                                        BSONObj query,
                                        MROutput output) {
    BSONObjBuilder b;
    b.append("mapreduce", nsGetCollection(ns));
    b.appendCode("map", jsmapf);
    b.appendCode("reduce", jsreducef);
    if (!query.isEmpty())
        b.append("query", query);
    b.append("out", output.out);
    BSONObj info;
    runCommand(nsGetDB(ns), b.done(), info);
    return info;
}

bool DBClientWithCommands::eval(const string& dbname,
                                const string& jscode,
                                BSONObj& info,
                                BSONElement& retValue,
                                BSONObj* args) {
    BSONObjBuilder b;
    b.appendCode("$eval", jscode);
    if (args)
        b.appendArray("args", *args);
    bool ok = runCommand(dbname, b.done(), info);
    if (ok)
        retValue = info.getField("retval");
    return ok;
}

bool DBClientWithCommands::eval(const string& dbname, const string& jscode) {
    BSONObj info;
    BSONElement retValue;
    return eval(dbname, jscode, info, retValue);
}

list<string> DBClientWithCommands::getDatabaseNames() {
    BSONObj info;
    uassert(10005,
            "listdatabases failed",
            runCommand("admin", BSON("listDatabases" << 1), info, QueryOption_SlaveOk));
    uassert(10006, "listDatabases.databases not array", info["databases"].type() == Array);

    list<string> names;

    BSONObjIterator i(info["databases"].embeddedObjectUserCheck());
    while (i.more()) {
        names.push_back(i.next().embeddedObjectUserCheck()["name"].valuestr());
    }

    return names;
}

list<string> DBClientWithCommands::getCollectionNames(const string& db) {
    list<BSONObj> infos = getCollectionInfos(db);
    list<string> names;
    for (list<BSONObj>::iterator it = infos.begin(); it != infos.end(); ++it) {
        names.push_back(db + "." + (*it)["name"].valuestr());
    }
    return names;
}

list<BSONObj> DBClientWithCommands::getCollectionInfos(const string& db, const BSONObj& filter) {
    list<BSONObj> infos;

    // first we're going to try the command
    // it was only added in 3.0, so if we're talking to an older server
    // we'll fail back to querying system.namespaces
    // TODO(spencer): remove fallback behavior after 3.0

    {
        BSONObj res;
        if (runCommand(db,
                       BSON("listCollections" << 1 << "filter" << filter << "cursor" << BSONObj()),
                       res,
                       QueryOption_SlaveOk)) {
            BSONObj cursorObj = res["cursor"].Obj();
            BSONObj collections = cursorObj["firstBatch"].Obj();
            BSONObjIterator it(collections);
            while (it.more()) {
                BSONElement e = it.next();
                infos.push_back(e.Obj().getOwned());
            }

            const long long id = cursorObj["id"].Long();

            if (id != 0) {
                const std::string ns = cursorObj["ns"].String();
                auto_ptr<DBClientCursor> cursor = getMore(ns, id, 0, 0);
                while (cursor->more()) {
                    infos.push_back(cursor->nextSafe().getOwned());
                }
            }

            return infos;
        }

        // command failed

        int code = res["code"].numberInt();
        string errmsg = res["errmsg"].valuestrsafe();
        if (code == ErrorCodes::CommandNotFound || errmsg.find("no such cmd") != string::npos) {
            // old version of server, ok, fall through to old code
        } else {
            uasserted(18630, str::stream() << "listCollections failed: " << res);
        }
    }

    // SERVER-14951 filter for old version fallback needs to db qualify the 'name' element
    BSONObjBuilder fallbackFilter;
    if (filter.hasField("name") && filter["name"].type() == String) {
        fallbackFilter.append("name", db + "." + filter["name"].str());
    }
    fallbackFilter.appendElementsUnique(filter);

    string ns = db + ".system.namespaces";
    auto_ptr<DBClientCursor> c =
        query(ns.c_str(), fallbackFilter.obj(), 0, 0, 0, QueryOption_SlaveOk);
    uassert(28611, str::stream() << "listCollections failed querying " << ns, c.get());

    while (c->more()) {
        BSONObj obj = c->nextSafe();
        string ns = obj["name"].valuestr();
        if (ns.find("$") != string::npos)
            continue;
        BSONObjBuilder b;
        b.append("name", ns.substr(db.size() + 1));
        b.appendElementsUnique(obj);
        infos.push_back(b.obj());
    }

    return infos;
}

bool DBClientWithCommands::exists(const string& ns) {
    BSONObj filter = BSON("name" << nsToCollectionSubstring(ns));
    list<BSONObj> results = getCollectionInfos(nsToDatabase(ns), filter);
    return !results.empty();
}

/* --- dbclientconnection --- */

void DBClientConnection::_auth(const BSONObj& params) {
    if (autoReconnect) {
        /* note we remember the auth info before we attempt to auth -- if the connection is broken,
         * we will then have it for the next autoreconnect attempt.
        */
        authCache[params[saslCommandUserDBFieldName].str()] = params.getOwned();
    }

    DBClientBase::_auth(params);
}

/** query N objects from the database into an array.  makes sense mostly when you want a small
 * number of results.  if a huge number, use
    query() and iterate the cursor.
 */
void DBClientInterface::findN(vector<BSONObj>& out,
                              const string& ns,
                              Query query,
                              int nToReturn,
                              int nToSkip,
                              const BSONObj* fieldsToReturn,
                              int queryOptions) {
    out.reserve(nToReturn);

    auto_ptr<DBClientCursor> c =
        this->query(ns, query, nToReturn, nToSkip, fieldsToReturn, queryOptions);

    uassert(10276,
            str::stream() << "DBClientBase::findN: transport error: " << getServerAddress()
                          << " ns: " << ns << " query: " << query.toString(),
            c.get());

    if (c->hasResultFlag(ResultFlag_ShardConfigStale)) {
        BSONObj error;
        c->peekError(&error);
        throw RecvStaleConfigException("findN stale config", error);
    }

    for (int i = 0; i < nToReturn; i++) {
        if (!c->more())
            break;
        out.push_back(c->nextSafe().copy());
    }
}

BSONObj DBClientInterface::findOne(const string& ns,
                                   const Query& query,
                                   const BSONObj* fieldsToReturn,
                                   int queryOptions) {
    vector<BSONObj> v;
    findN(v, ns, query, 1, 0, fieldsToReturn, queryOptions);
    return v.empty() ? BSONObj() : v[0];
}

bool DBClientConnection::connect(const HostAndPort& server, string& errmsg) {
    _server = server;
    _serverString = _server.toString();
    return _connect(errmsg);
}

bool DBClientConnection::_connect(string& errmsg) {
    _serverString = _server.toString();
    _serverAddrString.clear();

    // we keep around SockAddr for connection life -- maybe MessagingPort
    // requires that?
    std::auto_ptr<SockAddr> serverSockAddr(new SockAddr(_server.host().c_str(), _server.port()));
    if (!serverSockAddr->isValid()) {
        errmsg = str::stream() << "couldn't initialize connection to host "
                               << _server.host().c_str() << ", address is invalid";
        return false;
    }

    server.reset(serverSockAddr.release());
    p.reset(new MessagingPort(_so_timeout, _logLevel));

    if (_server.host().empty()) {
        errmsg = str::stream() << "couldn't connect to server " << toString() << ", host is empty";
        return false;
    }

    _serverAddrString = server->getAddr();

    if (_serverAddrString == "0.0.0.0") {
        errmsg = str::stream() << "couldn't connect to server " << toString()
                               << ", address resolved to 0.0.0.0";
        return false;
    }

    if (!p->connect(*server)) {
        errmsg = str::stream() << "couldn't connect to server " << toString()
                               << ", connection attempt failed";
        _failed = true;
        return false;
    } else {
        LOG(1) << "connected to server " << toString() << endl;
    }

#ifdef MONGO_SSL
    int sslModeVal = sslGlobalParams.sslMode.load();
    if (sslModeVal == SSLGlobalParams::SSLMode_preferSSL ||
        sslModeVal == SSLGlobalParams::SSLMode_requireSSL) {
        return p->secure(sslManager(), _server.host());
    }
#endif

    return true;
}

void DBClientConnection::logout(const string& dbname, BSONObj& info) {
    authCache.erase(dbname);
    runCommand(dbname, BSON("logout" << 1), info);
}

bool DBClientConnection::runCommand(const string& dbname,
                                    const BSONObj& cmd,
                                    BSONObj& info,
                                    int options) {
    if (DBClientWithCommands::runCommand(dbname, cmd, info, options))
        return true;

    if (!_parentReplSetName.empty()) {
        handleNotMasterResponse(info["errmsg"]);
    }

    return false;
}

void DBClientConnection::_checkConnection() {
    if (!_failed)
        return;

    if (!autoReconnect)
        throw SocketException(SocketException::FAILED_STATE, toString());

    // Don't hammer reconnects, backoff if needed
    autoReconnectBackoff.nextSleepMillis();

    LOG(_logLevel) << "trying reconnect to " << toString() << endl;
    string errmsg;
    _failed = false;
    if (!_connect(errmsg)) {
        _failed = true;
        LOG(_logLevel) << "reconnect " << toString() << " failed " << errmsg << endl;
        throw SocketException(SocketException::CONNECT_ERROR, toString());
    }

    LOG(_logLevel) << "reconnect " << toString() << " ok" << endl;
    for (map<string, BSONObj>::const_iterator i = authCache.begin(); i != authCache.end(); i++) {
        try {
            DBClientConnection::_auth(i->second);
        } catch (UserException& ex) {
            if (ex.getCode() != ErrorCodes::AuthenticationFailed)
                throw;
            LOG(_logLevel) << "reconnect: auth failed " << i->second[saslCommandUserDBFieldName]
                           << i->second[saslCommandUserFieldName] << ' ' << ex.what() << std::endl;
        }
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
    } else {
        return INVALID_SOCK_CREATION_TIME;
    }
}

const uint64_t DBClientBase::INVALID_SOCK_CREATION_TIME =
    static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL);

auto_ptr<DBClientCursor> DBClientBase::query(const string& ns,
                                             Query query,
                                             int nToReturn,
                                             int nToSkip,
                                             const BSONObj* fieldsToReturn,
                                             int queryOptions,
                                             int batchSize) {
    auto_ptr<DBClientCursor> c(new DBClientCursor(
        this, ns, query.obj, nToReturn, nToSkip, fieldsToReturn, queryOptions, batchSize));
    if (c->init())
        return c;
    return auto_ptr<DBClientCursor>(0);
}

auto_ptr<DBClientCursor> DBClientBase::getMore(const string& ns,
                                               long long cursorId,
                                               int nToReturn,
                                               int options) {
    auto_ptr<DBClientCursor> c(new DBClientCursor(this, ns, cursorId, nToReturn, options));
    if (c->init())
        return c;
    return auto_ptr<DBClientCursor>(0);
}

struct DBClientFunConvertor {
    void operator()(DBClientCursorBatchIterator& i) {
        while (i.moreInCurrentBatch()) {
            _f(i.nextSafe());
        }
    }
    stdx::function<void(const BSONObj&)> _f;
};

unsigned long long DBClientBase::query(stdx::function<void(const BSONObj&)> f,
                                       const string& ns,
                                       Query query,
                                       const BSONObj* fieldsToReturn,
                                       int queryOptions) {
    DBClientFunConvertor fun;
    fun._f = f;
    stdx::function<void(DBClientCursorBatchIterator&)> ptr(fun);
    return this->query(ptr, ns, query, fieldsToReturn, queryOptions);
}

unsigned long long DBClientBase::query(stdx::function<void(DBClientCursorBatchIterator&)> f,
                                       const string& ns,
                                       Query query,
                                       const BSONObj* fieldsToReturn,
                                       int queryOptions) {
    // mask options
    queryOptions &= (int)(QueryOption_NoCursorTimeout | QueryOption_SlaveOk);

    auto_ptr<DBClientCursor> c(this->query(ns, query, 0, 0, fieldsToReturn, queryOptions));
    uassert(16090, "socket error for mapping query", c.get());

    unsigned long long n = 0;

    while (c->more()) {
        DBClientCursorBatchIterator i(*c);
        f(i);
        n += i.n();
    }
    return n;
}

unsigned long long DBClientConnection::query(stdx::function<void(DBClientCursorBatchIterator&)> f,
                                             const string& ns,
                                             Query query,
                                             const BSONObj* fieldsToReturn,
                                             int queryOptions) {
    if (!(availableOptions() & QueryOption_Exhaust)) {
        return DBClientBase::query(f, ns, query, fieldsToReturn, queryOptions);
    }

    // mask options
    queryOptions &= (int)(QueryOption_NoCursorTimeout | QueryOption_SlaveOk);
    queryOptions |= (int)QueryOption_Exhaust;

    auto_ptr<DBClientCursor> c(this->query(ns, query, 0, 0, fieldsToReturn, queryOptions));
    uassert(13386, "socket error for mapping query", c.get());

    unsigned long long n = 0;

    try {
        while (1) {
            while (c->moreInCurrentBatch()) {
                DBClientCursorBatchIterator i(*c);
                f(i);
                n += i.n();
            }

            if (c->getCursorId() == 0)
                break;

            c->exhaustReceiveMore();
        }
    } catch (std::exception&) {
        /* connection CANNOT be used anymore as more data may be on the way from the server.
           we have to reconnect.
           */
        _failed = true;
        p->shutdown();
        throw;
    }

    return n;
}

void DBClientBase::insert(const string& ns, BSONObj obj, int flags) {
    Message toSend;

    BufBuilder b;

    int reservedFlags = 0;
    if (flags & InsertOption_ContinueOnError)
        reservedFlags |= Reserved_InsertOption_ContinueOnError;

    if (flags & WriteOption_FromWriteback)
        reservedFlags |= Reserved_FromWriteback;

    b.appendNum(reservedFlags);
    b.appendStr(ns);
    obj.appendSelfToBufBuilder(b);

    toSend.setData(dbInsert, b.buf(), b.len());

    say(toSend);
}

// TODO: Merge with other insert implementation?
void DBClientBase::insert(const string& ns, const vector<BSONObj>& v, int flags) {
    Message toSend;

    BufBuilder b;

    int reservedFlags = 0;
    if (flags & InsertOption_ContinueOnError)
        reservedFlags |= Reserved_InsertOption_ContinueOnError;

    if (flags & WriteOption_FromWriteback) {
        reservedFlags |= Reserved_FromWriteback;
        flags ^= WriteOption_FromWriteback;
    }

    b.appendNum(reservedFlags);
    b.appendStr(ns);
    for (vector<BSONObj>::const_iterator i = v.begin(); i != v.end(); ++i)
        i->appendSelfToBufBuilder(b);

    toSend.setData(dbInsert, b.buf(), b.len());

    say(toSend);
}

void DBClientBase::remove(const string& ns, Query obj, bool justOne) {
    int flags = 0;
    if (justOne)
        flags |= RemoveOption_JustOne;
    remove(ns, obj, flags);
}

void DBClientBase::remove(const string& ns, Query obj, int flags) {
    Message toSend;

    BufBuilder b;
    int reservedFlags = 0;
    if (flags & WriteOption_FromWriteback) {
        reservedFlags |= WriteOption_FromWriteback;
        flags ^= WriteOption_FromWriteback;
    }

    b.appendNum(reservedFlags);
    b.appendStr(ns);
    b.appendNum(flags);

    obj.obj.appendSelfToBufBuilder(b);

    toSend.setData(dbDelete, b.buf(), b.len());

    say(toSend);
}

void DBClientBase::update(const string& ns, Query query, BSONObj obj, bool upsert, bool multi) {
    int flags = 0;
    if (upsert)
        flags |= UpdateOption_Upsert;
    if (multi)
        flags |= UpdateOption_Multi;
    update(ns, query, obj, flags);
}

void DBClientBase::update(const string& ns, Query query, BSONObj obj, int flags) {
    BufBuilder b;

    int reservedFlags = 0;
    if (flags & WriteOption_FromWriteback) {
        reservedFlags |= Reserved_FromWriteback;
        flags ^= WriteOption_FromWriteback;
    }

    b.appendNum(reservedFlags);  // reserved
    b.appendStr(ns);
    b.appendNum(flags);

    query.obj.appendSelfToBufBuilder(b);
    obj.appendSelfToBufBuilder(b);

    Message toSend;
    toSend.setData(dbUpdate, b.buf(), b.len());

    say(toSend);
}

list<BSONObj> DBClientWithCommands::getIndexSpecs(const string& ns, int options) {
    list<BSONObj> specs;

    {
        BSONObj cmd = BSON("listIndexes" << nsToCollectionSubstring(ns) << "cursor" << BSONObj());

        BSONObj res;
        if (runCommand(nsToDatabase(ns), cmd, res, options)) {
            BSONObj cursorObj = res["cursor"].Obj();
            BSONObjIterator i(cursorObj["firstBatch"].Obj());
            while (i.more()) {
                specs.push_back(i.next().Obj().getOwned());
            }

            const long long id = cursorObj["id"].Long();

            if (id != 0) {
                const std::string ns = cursorObj["ns"].String();
                auto_ptr<DBClientCursor> cursor = getMore(ns, id, 0, 0);
                while (cursor->more()) {
                    specs.push_back(cursor->nextSafe().getOwned());
                }
            }

            return specs;
        }
        int code = res["code"].numberInt();
        string errmsg = res["errmsg"].valuestrsafe();
        if (code == ErrorCodes::CommandNotFound || errmsg.find("no such cmd") != string::npos) {
            // old version of server, ok, fall through to old code
        } else if (code == ErrorCodes::NamespaceNotFound) {
            return specs;
        } else {
            uasserted(18631, str::stream() << "listIndexes failed: " << res);
        }
    }

    // fallback to querying system.indexes
    // TODO(spencer): Remove fallback behavior after 3.0
    auto_ptr<DBClientCursor> cursor =
        query(NamespaceString(ns).getSystemIndexesCollection(), BSON("ns" << ns), 0, 0, 0, options);
    uassert(28612, str::stream() << "listIndexes failed querying " << ns, cursor.get());

    while (cursor->more()) {
        BSONObj spec = cursor->nextSafe();
        specs.push_back(spec.getOwned());
    }
    return specs;
}


void DBClientWithCommands::dropIndex(const string& ns, BSONObj keys) {
    dropIndex(ns, genIndexName(keys));
}


void DBClientWithCommands::dropIndex(const string& ns, const string& indexName) {
    BSONObj info;
    if (!runCommand(nsToDatabase(ns),
                    BSON("deleteIndexes" << nsToCollectionSubstring(ns) << "index" << indexName),
                    info)) {
        LOG(_logLevel) << "dropIndex failed: " << info << endl;
        uassert(10007, "dropIndex failed", 0);
    }
    resetIndexCache();
}

void DBClientWithCommands::dropIndexes(const string& ns) {
    BSONObj info;
    uassert(10008,
            "dropIndexes failed",
            runCommand(nsToDatabase(ns),
                       BSON("deleteIndexes" << nsToCollectionSubstring(ns) << "index"
                                            << "*"),
                       info));
    resetIndexCache();
}

void DBClientWithCommands::reIndex(const string& ns) {
    resetIndexCache();
    BSONObj info;
    uassert(18908,
            str::stream() << "reIndex failed: " << info,
            runCommand(nsToDatabase(ns), BSON("reIndex" << nsToCollectionSubstring(ns)), info));
}


string DBClientWithCommands::genIndexName(const BSONObj& keys) {
    stringstream ss;

    bool first = 1;
    for (BSONObjIterator i(keys); i.more();) {
        BSONElement f = i.next();

        if (first)
            first = 0;
        else
            ss << "_";

        ss << f.fieldName() << "_";
        if (f.isNumber())
            ss << f.numberInt();
        else
            ss << f.str();  // this should match up with shell command
    }
    return ss.str();
}

bool DBClientWithCommands::ensureIndex(const string& ns,
                                       BSONObj keys,
                                       bool unique,
                                       const string& name,
                                       bool cache,
                                       bool background,
                                       int version,
                                       int ttl) {
    BSONObjBuilder toSave;
    toSave.append("ns", ns);
    toSave.append("key", keys);

    string cacheKey(ns);
    cacheKey += "--";

    if (name != "") {
        toSave.append("name", name);
        cacheKey += name;
    } else {
        string nn = genIndexName(keys);
        toSave.append("name", nn);
        cacheKey += nn;
    }

    if (version >= 0)
        toSave.append("v", version);

    if (unique)
        toSave.appendBool("unique", unique);

    if (background)
        toSave.appendBool("background", true);

    if (_seenIndexes.count(cacheKey))
        return 0;

    if (cache)
        _seenIndexes.insert(cacheKey);

    if (ttl > 0)
        toSave.append("expireAfterSeconds", ttl);

    insert(NamespaceString(ns).getSystemIndexesCollection(), toSave.obj());
    return 1;
}

void DBClientWithCommands::resetIndexCache() {
    _seenIndexes.clear();
}

/* -- DBClientCursor ---------------------------------------------- */

#ifdef _DEBUG
#define CHECK_OBJECT(o, msg) massert(10337, (string) "object not valid" + (msg), (o).isValid())
#else
#define CHECK_OBJECT(o, msg)
#endif

void assembleRequest(const string& ns,
                     BSONObj query,
                     int nToReturn,
                     int nToSkip,
                     const BSONObj* fieldsToReturn,
                     int queryOptions,
                     Message& toSend) {
    CHECK_OBJECT(query, "assembleRequest query");
    // see query.h for the protocol we are using here.
    BufBuilder b;
    int opts = queryOptions;
    b.appendNum(opts);
    b.appendStr(ns);
    b.appendNum(nToSkip);
    b.appendNum(nToReturn);
    query.appendSelfToBufBuilder(b);
    if (fieldsToReturn)
        fieldsToReturn->appendSelfToBufBuilder(b);
    toSend.setData(dbQuery, b.buf(), b.len());
}

DBClientConnection::DBClientConnection(bool _autoReconnect, double so_timeout)
    : _failed(false),
      autoReconnect(_autoReconnect),
      autoReconnectBackoff(1000, 2000),
      _so_timeout(so_timeout) {
    _numConnections.fetchAndAdd(1);
}

void DBClientConnection::say(Message& toSend, bool isRetry, string* actualServer) {
    checkConnection();
    try {
        port().say(toSend);
    } catch (SocketException&) {
        _failed = true;
        throw;
    }
}

void DBClientConnection::sayPiggyBack(Message& toSend) {
    port().piggyBack(toSend);
}

bool DBClientConnection::recv(Message& m) {
    if (port().recv(m)) {
        return true;
    }

    _failed = true;
    return false;
}

bool DBClientConnection::call(Message& toSend,
                              Message& response,
                              bool assertOk,
                              string* actualServer) {
    /* todo: this is very ugly messagingport::call returns an error code AND can throw
             an exception.  we should make it return void and just throw an exception anytime
             it fails
    */
    checkConnection();
    try {
        if (!port().call(toSend, response)) {
            _failed = true;
            if (assertOk)
                uasserted(10278,
                          str::stream() << "dbclient error communicating with server: "
                                        << getServerAddress());

            return false;
        }
    } catch (SocketException&) {
        _failed = true;
        throw;
    }
    return true;
}

BSONElement getErrField(const BSONObj& o) {
    BSONElement first = o.firstElement();
    if (strcmp(first.fieldName(), "$err") == 0)
        return first;

    // temp - will be DEV only later
    /*DEV*/
    if (1) {
        BSONElement e = o["$err"];
        if (!e.eoo()) {
            wassert(false);
        }
        return e;
    }

    return BSONElement();
}

bool hasErrField(const BSONObj& o) {
    return !getErrField(o).eoo();
}

void DBClientConnection::checkResponse(const char* data, int nReturned, bool* retry, string* host) {
    /* check for errors.  the only one we really care about at
     * this stage is "not master"
    */

    *retry = false;
    *host = _serverString;

    if (!_parentReplSetName.empty() && nReturned) {
        verify(data);
        BSONObj bsonView(data);
        handleNotMasterResponse(getErrField(bsonView));
    }
}

void DBClientConnection::killCursor(long long cursorId) {
    StackBufBuilder b;
    b.appendNum((int)0);  // reserved
    b.appendNum((int)1);  // number
    b.appendNum(cursorId);

    Message m;
    m.setData(dbKillCursors, b.buf(), b.len());

    if (_lazyKillCursor)
        sayPiggyBack(m);
    else
        say(m);
}

void DBClientConnection::setParentReplSetName(const string& replSetName) {
    _parentReplSetName = replSetName;
}

void DBClientConnection::handleNotMasterResponse(const BSONElement& elemToCheck) {
    if (!isNotMasterErrorString(elemToCheck)) {
        return;
    }

    MONGO_LOG_COMPONENT(1, logger::LogComponent::kReplication)
        << "got not master from: " << _serverString << " of repl set: " << _parentReplSetName;

    ReplicaSetMonitorPtr monitor = ReplicaSetMonitor::get(_parentReplSetName);
    if (monitor) {
        monitor->failedHost(_server);
    }

    _failed = true;
}

#ifdef MONGO_SSL
static SimpleMutex s_mtx("SSLManager");
static SSLManagerInterface* s_sslMgr(NULL);

SSLManagerInterface* DBClientConnection::sslManager() {
    SimpleMutex::scoped_lock lk(s_mtx);
    if (s_sslMgr)
        return s_sslMgr;
    s_sslMgr = getSSLManager();

    return s_sslMgr;
}
#endif

AtomicInt32 DBClientConnection::_numConnections;
bool DBClientConnection::_lazyKillCursor = true;


bool serverAlive(const string& uri) {
    // potentially the connection to server could fail while we're checking if it's alive - so use
    // timeouts
    DBClientConnection c(false, 20);
    string err;
    if (!c.connect(HostAndPort(uri), err))
        return false;
    if (!c.simpleCommand("admin", 0, "ping"))
        return false;
    return true;
}


/** @return the database name portion of an ns string */
string nsGetDB(const string& ns) {
    string::size_type pos = ns.find(".");
    if (pos == string::npos)
        return ns;

    return ns.substr(0, pos);
}

/** @return the collection name portion of an ns string */
string nsGetCollection(const string& ns) {
    string::size_type pos = ns.find(".");
    if (pos == string::npos)
        return "";

    return ns.substr(pos + 1);
}


}  // namespace mongo
