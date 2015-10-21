/**
 *    Copyright (C) 2013 MongoDB Inc.
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

#include "mongo/s/dbclient_multi_command.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/bson/mutable/document.h"
#include "mongo/db/audit.h"
#include "mongo/db/client_basic.h"
#include "mongo/db/dbmessage.h"
#include "mongo/db/wire_version.h"
#include "mongo/s/shard.h"
#include "mongo/s/write_ops/batched_command_request.h"
#include "mongo/db/server_parameters.h"
#include "mongo/util/net/message.h"

namespace mongo {

using boost::scoped_ptr;
using std::deque;
using std::string;

DBClientMultiCommand::PendingCommand::PendingCommand(const ConnectionString& endpoint,
                                                     const StringData& dbName,
                                                     const BSONObj& cmdObj)
    : endpoint(endpoint),
      dbName(dbName.toString()),
      cmdObj(cmdObj),
      conn(NULL),
      status(Status::OK()) {}

void DBClientMultiCommand::addCommand(const ConnectionString& endpoint,
                                      const StringData& dbName,
                                      const BSONSerializable& request) {
    PendingCommand* command = new PendingCommand(endpoint, dbName, request.toBSON());
    _pendingCommands.push_back(command);
}

namespace {

//
// For sanity checks of batch write operations
//

BatchedCommandRequest::BatchType getBatchWriteType(const BSONObj& cmdObj) {
    string cmdName = cmdObj.firstElement().fieldName();
    if (cmdName == "insert")
        return BatchedCommandRequest::BatchType_Insert;
    if (cmdName == "update")
        return BatchedCommandRequest::BatchType_Update;
    if (cmdName == "delete")
        return BatchedCommandRequest::BatchType_Delete;
    return BatchedCommandRequest::BatchType_Unknown;
}

bool isBatchWriteCommand(const BSONObj& cmdObj) {
    return getBatchWriteType(cmdObj) != BatchedCommandRequest::BatchType_Unknown;
}

bool hasBatchWriteFeature(DBClientBase* conn) {
    return conn->getMinWireVersion() <= BATCH_COMMANDS &&
        conn->getMaxWireVersion() >= BATCH_COMMANDS;
}
}

// THROWS
static void sayAsCmd(DBClientBase* conn, const StringData& dbName, const BSONObj& cmdObj) {
    Message toSend;
    BSONObjBuilder usersBuilder;
    usersBuilder.appendElements(cmdObj);
    audit::appendImpersonatedUsers(&usersBuilder);

    // see query.h for the protocol we are using here.
    BufBuilder bufB;
    bufB.appendNum(0);                            // command/query options
    bufB.appendStr(dbName.toString() + ".$cmd");  // write command ns
    bufB.appendNum(0);                            // ntoskip (0 for command)
    bufB.appendNum(1);                            // ntoreturn (1 for command)
    usersBuilder.obj().appendSelfToBufBuilder(bufB);
    toSend.setData(dbQuery, bufB.buf(), bufB.len());

    // Send our command
    conn->say(toSend);
}

// THROWS
static void recvAsCmd(DBClientBase* conn, Message* toRecv, BSONObj* result) {
    if (!conn->recv(*toRecv)) {
        // Confusingly, socket exceptions here are written to the log, not thrown.
        uasserted(17255,
                  "error receiving write command response, "
                  "possible socket exception - see logs");
    }

    // A query result is returned from commands
    QueryResult::View recvdQuery = toRecv->singleData().view2ptr();
    *result = BSONObj(recvdQuery.data());
}

void DBClientMultiCommand::sendAll() {
    for (deque<PendingCommand*>::iterator it = _pendingCommands.begin();
         it != _pendingCommands.end();
         ++it) {
        PendingCommand* command = *it;
        dassert(NULL == command->conn);

        try {
            dassert(command->endpoint.type() == ConnectionString::MASTER ||
                    command->endpoint.type() == ConnectionString::CUSTOM);

            // TODO: Fix the pool up to take millis directly
            int timeoutSecs = _timeoutMillis / 1000;
            command->conn = shardConnectionPool.get(command->endpoint, timeoutSecs);

            // Sanity check if we're sending a batch write that we're talking to a new-enough
            // server.
            massert(28563,
                    str::stream() << "cannot send batch write operation to server "
                                  << command->conn->toString(),
                    !isBatchWriteCommand(command->cmdObj) || hasBatchWriteFeature(command->conn));

            sayAsCmd(command->conn, command->dbName, command->cmdObj);
        } catch (const DBException& ex) {
            command->status = ex.toStatus();

            if (NULL != command->conn) {
                // Confusingly, the pool needs to know about failed connections so that it can
                // invalidate other connections which might be bad.  But if the connection
                // doesn't seem bad, don't send it back, because we don't want to reuse it.
                if (!command->conn->isFailed()) {
                    delete command->conn;
                } else {
                    shardConnectionPool.release(command->endpoint.toString(), command->conn);
                }

                command->conn = NULL;
            }
        }
    }
}

int DBClientMultiCommand::numPending() const {
    return static_cast<int>(_pendingCommands.size());
}

Status DBClientMultiCommand::recvAny(ConnectionString* endpoint, BSONSerializable* response) {
    scoped_ptr<PendingCommand> command(_pendingCommands.front());
    _pendingCommands.pop_front();

    *endpoint = command->endpoint;
    if (!command->status.isOK())
        return command->status;

    dassert(NULL != command->conn);

    try {
        // Holds the data and BSONObj for the command result
        Message toRecv;
        BSONObj result;

        recvAsCmd(command->conn, &toRecv, &result);

        shardConnectionPool.release(command->endpoint.toString(), command->conn);
        command->conn = NULL;

        string errMsg;
        if (!response->parseBSON(result, &errMsg) || !response->isValid(&errMsg)) {
            return Status(ErrorCodes::FailedToParse, errMsg);
        }
    } catch (const DBException& ex) {
        // Confusingly, the pool needs to know about failed connections so that it can
        // invalidate other connections which might be bad.  But if the connection doesn't seem
        // bad, don't send it back, because we don't want to reuse it.
        if (!command->conn->isFailed()) {
            delete command->conn;
        } else {
            shardConnectionPool.release(command->endpoint.toString(), command->conn);
        }
        command->conn = NULL;

        return ex.toStatus();
    }

    return Status::OK();
}

DBClientMultiCommand::~DBClientMultiCommand() {
    // Cleanup anything outstanding, do *not* return stuff to the pool, that might error
    for (deque<PendingCommand*>::iterator it = _pendingCommands.begin();
         it != _pendingCommands.end();
         ++it) {
        PendingCommand* command = *it;

        if (NULL != command->conn)
            delete command->conn;
        delete command;
        command = NULL;
    }

    _pendingCommands.clear();
}

void DBClientMultiCommand::setTimeoutMillis(int milliSecs) {
    _timeoutMillis = milliSecs;
}
}
