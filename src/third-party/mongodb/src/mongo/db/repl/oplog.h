/**
*    Copyright (C) 2008 10gen Inc.
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
*    must comply with the GNU Affero General Public License in all respects for
*    all of the code used other than as permitted herein. If you modify file(s)
*    with this exception, you may extend this exception to your version of the
*    file(s), but you are not obligated to do so. If you do not wish to do so,
*    delete this exception statement from your version. If you delete this
*    exception statement from all source files in the program, then also delete
*    it in the license file.
*/

#pragma once

#include <cstddef>
#include <deque>
#include <string>

namespace mongo {
class BSONObj;
class Database;
class OperationContext;
class OpTime;

namespace repl {

// Redefines the function for logOp() to master/slave.
void oldRepl();  // master-slave

// Create a new capped collection for the oplog if it doesn't yet exist.
// This will be either local.oplog.rs (replica sets) or local.oplog.$main (master/slave)
// If the collection already exists, set the 'last' OpTime if master/slave (side effect!)
void createOplog(OperationContext* txn);

// This function writes ops into the replica-set oplog;
// used internally by replication secondaries after they have applied ops.  Updates the global
// optime.
// Returns the optime for the last op inserted.
OpTime writeOpsToOplog(OperationContext* txn, const std::deque<BSONObj>& ops);

const char rsoplog[] = "local.oplog.rs";
static const int OPLOG_VERSION = 2;

/** Log an operation to the local oplog

   @param opstr
    "i" insert
    "u" update
    "d" delete
    "c" db cmd
    "n" no-op
    "db" declares presence of a database (ns is set to the db name + '.')

   For 'u' records, 'obj' captures the mutation made to the object but not
   the object itself. In that case, we provide also 'fullObj' which is the
   image of the object _after_ the mutation logged here was applied.

   See _logOp() in oplog.cpp for more details.
*/
void logOp(OperationContext* txn,
           const char* opstr,
           const char* ns,
           const BSONObj& obj,
           BSONObj* patt = NULL,
           bool* b = NULL,
           bool fromMigrate = false);

// Log an empty no-op operation to the local oplog
void logKeepalive(OperationContext* txn);

/** puts obj in the oplog as a comment (a no-op).  Just for diags.
    convention is
      { msg : "text", ... }
*/
void logOpComment(OperationContext* txn, const BSONObj& obj);

// Same as logOpComment, except only works for replsets
void logOpInitiate(OperationContext* txn, const BSONObj& obj);

// Flush out the cached pointers to the local database and oplog.
// Used by the closeDatabase command to ensure we don't cache closed things.
void oplogCheckCloseDatabase(OperationContext* txn, Database* db);

/**
 * take an op and apply locally
 * used for applying from an oplog
 * @param fromRepl really from replication or for testing/internal/command/etc...
 * @param convertUpdateToUpsert convert some updates to upserts for idempotency reasons
 * Returns if the op was an update that could not be applied (true on failure)
 */
bool applyOperation_inlock(OperationContext* txn,
                           Database* db,
                           const BSONObj& op,
                           bool fromRepl = true,
                           bool convertUpdateToUpsert = false);

/**
 * Waits one second for the OpTime from the oplog to change.
 */
void waitUpToOneSecondForOptimeChange(const OpTime& referenceTime);

/**
 * Initializes the global OpTime with the value from the timestamp of the last oplog entry.
 */
void initOpTimeFromOplog(OperationContext* txn, const std::string& oplogNS);

/**
 * Sets the global OpTime to be 'newTime'.
 */
void setNewOptime(const OpTime& newTime);
}  // namespace repl
}  // namespace mongo
