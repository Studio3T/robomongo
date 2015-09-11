/**
 *    Copyright (C) 2014 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kReplication

#include "mongo/platform/basic.h"

#include "mongo/db/repl/minvalid.h"

#include "mongo/bson/optime.h"
#include "mongo/db/concurrency/d_concurrency.h"
#include "mongo/db/concurrency/write_conflict_exception.h"
#include "mongo/db/dbhelpers.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/util/log.h"

namespace mongo {
namespace repl {

namespace {
const char* initialSyncFlagString = "doingInitialSync";
const BSONObj initialSyncFlag(BSON(initialSyncFlagString << true));
const char* minvalidNS = "local.replset.minvalid";
}  // namespace

void clearInitialSyncFlag(OperationContext* txn) {
    MONGO_WRITE_CONFLICT_RETRY_LOOP_BEGIN {
        ScopedTransaction transaction(txn, MODE_IX);
        Lock::DBLock lk(txn->lockState(), "local", MODE_X);
        Helpers::putSingleton(txn, minvalidNS, BSON("$unset" << initialSyncFlag));
    }
    MONGO_WRITE_CONFLICT_RETRY_LOOP_END(txn, "clearInitialSyncFlags", minvalidNS);
}

void setInitialSyncFlag(OperationContext* txn) {
    MONGO_WRITE_CONFLICT_RETRY_LOOP_BEGIN {
        ScopedTransaction transaction(txn, MODE_IX);
        Lock::DBLock lk(txn->lockState(), "local", MODE_X);
        Helpers::putSingleton(txn, minvalidNS, BSON("$set" << initialSyncFlag));
    }
    MONGO_WRITE_CONFLICT_RETRY_LOOP_END(txn, "setInitialSyncFlags", minvalidNS);
}

bool getInitialSyncFlag() {
    OperationContextImpl txn;
    MONGO_WRITE_CONFLICT_RETRY_LOOP_BEGIN {
        ScopedTransaction transaction(&txn, MODE_IX);
        Lock::DBLock lk(txn.lockState(), "local", MODE_X);
        BSONObj mv;
        bool found = Helpers::getSingleton(&txn, minvalidNS, mv);
        if (found) {
            return mv[initialSyncFlagString].trueValue();
        }
        return false;
    }
    MONGO_WRITE_CONFLICT_RETRY_LOOP_END(&txn, "getInitialSyncFlags", minvalidNS);
}

void setMinValid(OperationContext* ctx, OpTime ts) {
    MONGO_WRITE_CONFLICT_RETRY_LOOP_BEGIN {
        ScopedTransaction transaction(ctx, MODE_IX);
        Lock::DBLock lk(ctx->lockState(), "local", MODE_X);
        Helpers::putSingleton(ctx, minvalidNS, BSON("$set" << BSON("ts" << ts)));
    }
    MONGO_WRITE_CONFLICT_RETRY_LOOP_END(ctx, "setMinValid", minvalidNS);
}

OpTime getMinValid(OperationContext* txn) {
    MONGO_WRITE_CONFLICT_RETRY_LOOP_BEGIN {
        ScopedTransaction transaction(txn, MODE_IS);
        Lock::DBLock lk(txn->lockState(), "local", MODE_S);
        BSONObj mv;
        bool found = Helpers::getSingleton(txn, minvalidNS, mv);
        if (found) {
            return mv["ts"]._opTime();
        }
        return OpTime();
    }
    MONGO_WRITE_CONFLICT_RETRY_LOOP_END(txn, "getMinValid", minvalidNS);
}
}
}
