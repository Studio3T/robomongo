/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kStorage

#include "mongo/platform/basic.h"

#include <boost/thread/mutex.hpp>
#include <set>

#include "mongo/base/checked_cast.h"
#include "mongo/db/client.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/catalog/database.h"
#include "mongo/db/concurrency/d_concurrency.h"
#include "mongo/db/global_environment_experiment.h"
#include "mongo/db/namespace_string.h"
#include "mongo/db/operation_context_impl.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_kv_engine.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_record_store.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_recovery_unit.h"
#include "mongo/util/background.h"
#include "mongo/util/exit.h"
#include "mongo/util/log.h"

namespace mongo {

namespace {

std::set<NamespaceString> _backgroundThreadNamespaces;
boost::mutex _backgroundThreadMutex;

class WiredTigerRecordStoreThread : public BackgroundJob {
public:
    WiredTigerRecordStoreThread(const NamespaceString& ns)
        : BackgroundJob(true /* deleteSelf */), _ns(ns) {
        _name = std::string("WiredTigerRecordStoreThread for ") + _ns.toString();
    }

    virtual std::string name() const {
        return _name;
    }

    /**
     * @return Number of documents deleted.
     */
    int64_t _deleteExcessDocuments() {
        if (!getGlobalEnvironment()->getGlobalStorageEngine()) {
            LOG(1) << "no global storage engine yet";
            return 0;
        }

        OperationContextImpl txn;
        checked_cast<WiredTigerRecoveryUnit*>(txn.recoveryUnit())->markNoTicketRequired();

        try {
            ScopedTransaction transaction(&txn, MODE_IX);

            AutoGetDb autoDb(&txn, _ns.db(), MODE_IX);
            Database* db = autoDb.getDb();
            if (!db) {
                LOG(2) << "no local database yet";
                return 0;
            }

            Lock::CollectionLock collectionLock(txn.lockState(), _ns.ns(), MODE_IX);
            Collection* collection = db->getCollection(_ns);
            if (!collection) {
                LOG(2) << "no collection " << _ns;
                return 0;
            }

            Client::Context ctx(&txn, _ns, false);
            WiredTigerRecordStore* rs =
                checked_cast<WiredTigerRecordStore*>(collection->getRecordStore());
            WriteUnitOfWork wuow(&txn);
            boost::timed_mutex::scoped_lock lock(rs->cappedDeleterMutex());
            int64_t removed = rs->cappedDeleteAsNeeded_inlock(&txn, RecordId::max());
            wuow.commit();
            return removed;
        } catch (const std::exception& e) {
            severe() << "error in WiredTigerRecordStoreThread: " << e.what();
            fassertFailedNoTrace(!"error in WiredTigerRecordStoreThread");
        } catch (...) {
            fassertFailedNoTrace(!"unknown error in WiredTigerRecordStoreThread");
        }
    }

    virtual void run() {
        Client::initThread(_name.c_str());

        while (!inShutdown()) {
            int64_t removed = _deleteExcessDocuments();
            LOG(2) << "WiredTigerRecordStoreThread deleted " << removed;
            if (removed == 0) {
                // If we removed 0 documents, sleep a bit in case we're on a laptop
                // or something to be nice.
                sleepmillis(1000);
            } else if (removed < 1000) {
                // 1000 is the batch size, so we didn't even do a full batch,
                // which is the most efficient.
                sleepmillis(10);
            }
        }

        cc().shutdown();

        log() << "shutting down";
    }

private:
    NamespaceString _ns;
    std::string _name;
};

}  // namespace

// static
bool WiredTigerKVEngine::initRsOplogBackgroundThread(StringData ns) {
    if (!NamespaceString::oplog(ns)) {
        return false;
    }

    if (storageGlobalParams.repair) {
        LOG(1) << "not starting WiredTigerRecordStoreThread for " << ns
               << " because we are in repair";
        return false;
    }

    boost::mutex::scoped_lock lock(_backgroundThreadMutex);
    NamespaceString nss(ns);
    if (_backgroundThreadNamespaces.count(nss)) {
        log() << "WiredTigerRecordStoreThread " << ns << " already started";
    } else {
        log() << "Starting WiredTigerRecordStoreThread " << ns;
        BackgroundJob* backgroundThread = new WiredTigerRecordStoreThread(nss);
        backgroundThread->go();
        _backgroundThreadNamespaces.insert(nss);
    }
    return true;
}

}  // namespace mongo
