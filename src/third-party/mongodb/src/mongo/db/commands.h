// commands.h

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

#include <string>
#include <vector>

#include "mongo/base/counter.h"
#include "mongo/base/status.h"
#include "mongo/db/auth/privilege.h"
#include "mongo/db/auth/resource_pattern.h"
#include "mongo/db/client_basic.h"
#include "mongo/db/commands/server_status_metric.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/query/explain.h"
#include "mongo/util/string_map.h"

namespace mongo {

class BSONObj;
class BSONObjBuilder;
class Client;
class Database;
class Timer;
class OperationContext;

namespace mutablebson {
class Document;
}  // namespace mutablebson

/** mongodb "commands" (sent via db.$cmd.findOne(...))
    subclass to make a command.  define a singleton object for it.
    */
class Command {
protected:
    // The type of the first field in 'cmdObj' must be mongo::String. The first field is
    // interpreted as a collection name.
    std::string parseNsFullyQualified(const std::string& dbname, const BSONObj& cmdObj) const;

    // The type of the first field in 'cmdObj' must be mongo::String or Symbol.
    // The first field is interpreted as a collection name.
    std::string parseNsCollectionRequired(const std::string& dbname, const BSONObj& cmdObj) const;

public:
    typedef StringMap<Command*> CommandMap;

    // Return the namespace for the command. If the first field in 'cmdObj' is of type
    // mongo::String, then that field is interpreted as the collection name, and is
    // appended to 'dbname' after a '.' character. If the first field is not of type
    // mongo::String, then 'dbname' is returned unmodified.
    virtual std::string parseNs(const std::string& dbname, const BSONObj& cmdObj) const;

    // Utility that returns a ResourcePattern for the namespace returned from
    // parseNs(dbname, cmdObj).  This will be either an exact namespace resource pattern
    // or a database resource pattern, depending on whether parseNs returns a fully qualifed
    // collection name or just a database name.
    ResourcePattern parseResourcePattern(const std::string& dbname, const BSONObj& cmdObj) const;

    const std::string name;

    /* run the given command
       implement this...

       fromRepl - command is being invoked as part of replication syncing.  In this situation you
                  normally do not want to log the command to the local oplog.

       return value is true if succeeded.  if false, set errmsg text.
    */
    virtual bool run(OperationContext* txn,
                     const std::string& db,
                     BSONObj& cmdObj,
                     int options,
                     std::string& errmsg,
                     BSONObjBuilder& result,
                     bool fromRepl = false) = 0;

    /**
     * This designation for the command is only used by the 'help' call and has nothing to do
     * with lock acquisition. The reason we need to have it there is because
     * SyncClusterConnection uses this to determine whether the command is update and needs to
     * be sent to all three servers or just one.
     *
     * Eventually when SyncClusterConnection is refactored out, we can get rid of it.
     */
    virtual bool isWriteCommandForConfigServer() const = 0;

    /* Return true if only the admin ns has privileges to run this command. */
    virtual bool adminOnly() const {
        return false;
    }

    void htmlHelp(std::stringstream&) const;

    /* Like adminOnly, but even stricter: we must either be authenticated for admin db,
       or, if running without auth, on the local interface.  Used for things which
       are so major that remote invocation may not make sense (e.g., shutdownServer).

       When localHostOnlyIfNoAuth() is true, adminOnly() must also be true.
    */
    virtual bool localHostOnlyIfNoAuth(const BSONObj& cmdObj) {
        return false;
    }

    /* Return true if slaves are allowed to execute the command
       (the command directly from a client -- if fromRepl, always allowed).
    */
    virtual bool slaveOk() const = 0;

    /* Return true if the client force a command to be run on a slave by
       turning on the 'slaveOk' option in the command query.
    */
    virtual bool slaveOverrideOk() const {
        return false;
    }

    /**
     * Override and return fales if the command opcounters should not be incremented on
     * behalf of this command.
     */
    virtual bool shouldAffectCommandCounter() const {
        return true;
    }

    virtual void help(std::stringstream& help) const;

    /**
     * Commands which can be explained override this method. Any operation which has a query
     * part and executes as a tree of execution stages can be explained. A command should
     * implement explain by:
     *
     *   1) Calling its custom parse function in order to parse the command. The output of
     *   this function should be a CanonicalQuery (representing the query part of the
     *   operation), and a PlanExecutor which wraps the tree of execution stages.
     *
     *   2) Calling Explain::explainStages(...) on the PlanExecutor. This is the function
     *   which knows how to convert an execution stage tree into explain output.
     */
    virtual Status explain(OperationContext* txn,
                           const std::string& dbname,
                           const BSONObj& cmdObj,
                           ExplainCommon::Verbosity verbosity,
                           BSONObjBuilder* out) const {
        return Status(ErrorCodes::IllegalOperation, "Cannot explain cmd: " + name);
    }

    /**
     * Checks if the given client is authorized to run this command on database "dbname"
     * with the invocation described by "cmdObj".
     */
    virtual Status checkAuthForCommand(ClientBasic* client,
                                       const std::string& dbname,
                                       const BSONObj& cmdObj);

    /**
     * Redacts "cmdObj" in-place to a form suitable for writing to logs.
     *
     * The default implementation does nothing.
     */
    virtual void redactForLogging(mutablebson::Document* cmdObj);

    /**
     * Returns a copy of "cmdObj" in a form suitable for writing to logs.
     * Uses redactForLogging() to transform "cmdObj".
     */
    BSONObj getRedactedCopyForLogging(const BSONObj& cmdObj);

    /* Return true if a replica set secondary should go into "recovering"
       (unreadable) state while running this command.
     */
    virtual bool maintenanceMode() const {
        return false;
    }

    /* Return true if command should be permitted when a replica set secondary is in "recovering"
       (unreadable) state.
     */
    virtual bool maintenanceOk() const {
        return true; /* assumed true prior to commit */
    }

    /** @param webUI expose the command in the web ui as localhost:28017/<name>
        @param oldName an optional old, deprecated name for the command
    */
    Command(StringData _name, bool webUI = false, StringData oldName = StringData());

    virtual ~Command() {}

protected:
    /**
     * Appends to "*out" the privileges required to run this command on database "dbname" with
     * the invocation described by "cmdObj".  New commands shouldn't implement this, they should
     * implement checkAuthForCommand instead.
     */
    virtual void addRequiredPrivileges(const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out) {
        // The default implementation of addRequiredPrivileges should never be hit.
        fassertFailed(16940);
    }

    BSONObj getQuery(const BSONObj& cmdObj) {
        if (cmdObj["query"].type() == Object)
            return cmdObj["query"].embeddedObject();
        if (cmdObj["q"].type() == Object)
            return cmdObj["q"].embeddedObject();
        return BSONObj();
    }

    static void logIfSlow(const Timer& cmdTimer, const std::string& msg);

    static CommandMap* _commands;
    static CommandMap* _commandsByBestName;
    static CommandMap* _webCommands;

    // Counters for how many times this command has been executed and failed
    Counter64 _commandsExecuted;
    Counter64 _commandsFailed;

    // Pointers to hold the metrics tree references
    ServerStatusMetricField<Counter64> _commandsExecutedMetric;
    ServerStatusMetricField<Counter64> _commandsFailedMetric;

public:
    static const CommandMap* commandsByBestName() {
        return _commandsByBestName;
    }
    static const CommandMap* webCommands() {
        return _webCommands;
    }

    // Counter for unknown commands
    static Counter64 unknownCommands;

    /** @return if command was found */
    static void runAgainstRegistered(const char* ns,
                                     BSONObj& jsobj,
                                     BSONObjBuilder& anObjBuilder,
                                     int queryOptions = 0);
    static Command* findCommand(const StringData& name);
    // For mongod and webserver.
    static void execCommand(OperationContext* txn,
                            Command* c,
                            int queryOptions,
                            const char* ns,
                            BSONObj& cmdObj,
                            BSONObjBuilder& result,
                            bool fromRepl);
    // For mongos
    static void execCommandClientBasic(OperationContext* txn,
                                       Command* c,
                                       ClientBasic& client,
                                       int queryOptions,
                                       const char* ns,
                                       BSONObj& cmdObj,
                                       BSONObjBuilder& result,
                                       bool fromRepl);

    // Helper for setting errmsg and ok field in command result object.
    static void appendCommandStatus(BSONObjBuilder& result, bool ok, const std::string& errmsg);

    // @return s.isOK()
    static bool appendCommandStatus(BSONObjBuilder& result, const Status& status);

    // Converts "result" into a Status object.  The input is expected to be the object returned
    // by running a command.  Returns ErrorCodes::CommandResultSchemaViolation if "result" does
    // not look like the result of a command.
    static Status getStatusFromCommandResult(const BSONObj& result);

    /**
     * Parses cursor options from the command request object "cmdObj".  Used by commands that
     * take cursor options.  The only cursor option currently supported is "cursor.batchSize".
     *
     * If a valid batch size was specified, returns Status::OK() and fills in "batchSize" with
     * the specified value.  If no batch size was specified, returns Status::OK() and fills in
     * "batchSize" with the provided default value.
     *
     * If an error occurred while parsing, returns an error Status.  If this is the case, the
     * value pointed to by "batchSize" is unspecified.
     */
    static Status parseCommandCursorOptions(const BSONObj& cmdObj,
                                            long long defaultBatchSize,
                                            long long* batchSize);

    /**
     * Builds a cursor response object from the provided cursor identifiers and "firstBatch",
     * and appends the response object to the provided builder under the field name "cursor".
     *
     * The response object has the following format:
     *   { id: <NumberLong>, ns: <String>, firstBatch: <Array> }.
     */
    static void appendCursorResponseObject(long long cursorId,
                                           StringData cursorNamespace,
                                           BSONArray firstBatch,
                                           BSONObjBuilder* builder);

    // Set by command line.  Controls whether or not testing-only commands should be available.
    static int testCommandsEnabled;

private:
    /**
     * Checks to see if the client is authorized to run the given command with the given
     * parameters on the given named database.
     *
     * fromRepl is true if this command is running as part of oplog application, which for
     * historic reasons has slightly different authorization semantics.  TODO(schwerin): Check
     * to see if this oddity can now be eliminated.
     *
     * Returns Status::OK() if the command is authorized.  Most likely returns
     * ErrorCodes::Unauthorized otherwise, but any return other than Status::OK implies not
     * authorized.
     */
    static Status _checkAuthorization(Command* c,
                                      ClientBasic* client,
                                      const std::string& dbname,
                                      const BSONObj& cmdObj,
                                      bool fromRepl);
};

bool _runCommands(OperationContext* txn,
                  const char* ns,
                  BSONObj& jsobj,
                  BufBuilder& b,
                  BSONObjBuilder& anObjBuilder,
                  bool fromRepl,
                  int queryOptions);

}  // namespace mongo
