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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kReplication

#include "mongo/platform/basic.h"

#include "mongo/db/repl/sync.h"

#include <string>

#include "mongo/db/catalog/collection.h"
#include "mongo/db/catalog/database.h"
#include "mongo/db/client.h"
#include "mongo/db/curop.h"
#include "mongo/db/concurrency/write_conflict_exception.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/record_id.h"
#include "mongo/db/repl/oplogreader.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/log.h"

namespace mongo {

using std::endl;
using std::string;

namespace repl {

void Sync::setHostname(const string& hostname) {
    hn = hostname;
}

BSONObj Sync::getMissingDoc(OperationContext* txn, Database* db, const BSONObj& o) {
    OplogReader missingObjReader;  // why are we using OplogReader to run a non-oplog query?
    const char* ns = o.getStringField("ns");

    // capped collections
    Collection* collection = db->getCollection(ns);
    if (collection && collection->isCapped()) {
        log() << "replication missing doc, but this is okay for a capped collection (" << ns << ")"
              << endl;
        return BSONObj();
    }

    const int retryMax = 3;
    for (int retryCount = 1; retryCount <= retryMax; ++retryCount) {
        if (retryCount != 1) {
            // if we are retrying, sleep a bit to let the network possibly recover
            sleepsecs(retryCount * retryCount);
        }
        try {
            bool ok = missingObjReader.connect(HostAndPort(hn));
            if (!ok) {
                warning() << "network problem detected while connecting to the "
                          << "sync source, attempt " << retryCount << " of " << retryMax << endl;
                continue;  // try again
            }
        } catch (const SocketException&) {
            warning() << "network problem detected while connecting to the "
                      << "sync source, attempt " << retryCount << " of " << retryMax << endl;
            continue;  // try again
        }

        // might be more than just _id in the update criteria
        BSONObj query = BSONObjBuilder().append(o.getObjectField("o2")["_id"]).obj();
        BSONObj missingObj;
        try {
            missingObj = missingObjReader.findOne(ns, query);
        } catch (const SocketException&) {
            warning() << "network problem detected while fetching a missing document from the "
                      << "sync source, attempt " << retryCount << " of " << retryMax << endl;
            continue;  // try again
        } catch (DBException& e) {
            log() << "replication assertion fetching missing object: " << e.what() << endl;
            throw;
        }

        // success!
        return missingObj;
    }
    // retry count exceeded
    msgasserted(15916, str::stream() << "Can no longer connect to initial sync source: " << hn);
}

bool Sync::shouldRetry(OperationContext* txn, const BSONObj& o) {
    const NamespaceString nss(o.getStringField("ns"));
    MONGO_WRITE_CONFLICT_RETRY_LOOP_BEGIN {
        // Take an X lock on the database in order to preclude other modifications.
        // Also, the database might not exist yet, so create it.
        AutoGetOrCreateDb autoDb(txn, nss.db(), MODE_X);
        Database* const db = autoDb.getDb();

        // we don't have the object yet, which is possible on initial sync.  get it.
        log() << "adding missing object" << endl;  // rare enough we can log
        BSONObj missingObj = getMissingDoc(txn, db, o);

        if (missingObj.isEmpty()) {
            log() << "missing object not found on source."
                     " presumably deleted later in oplog";
            log() << "o2: " << o.getObjectField("o2").toString();
            log() << "o firstfield: " << o.getObjectField("o").firstElementFieldName();
            return false;
        } else {
            WriteUnitOfWork wunit(txn);

            Collection* const coll = db->getOrCreateCollection(txn, nss.toString());
            invariant(coll);

            StatusWith<RecordId> result = coll->insertDocument(txn, missingObj, true);
            uassert(
                15917,
                str::stream() << "failed to insert missing doc: " << result.getStatus().toString(),
                result.isOK());
            LOG(1) << "inserted missing doc: " << missingObj.toString() << endl;
            wunit.commit();
            return true;
        }
    }
    MONGO_WRITE_CONFLICT_RETRY_LOOP_END(txn, "InsertRetry", nss.ns());
}

}  // namespace repl
}  // namespace mongo
