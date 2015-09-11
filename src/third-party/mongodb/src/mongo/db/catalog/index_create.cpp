// index_create.cpp

/**
*    Copyright (C) 2008-2014 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kIndex

#include "mongo/platform/basic.h"

#include "mongo/db/catalog/index_create.h"

#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>

#include "mongo/base/error_codes.h"
#include "mongo/client/dbclientinterface.h"
#include "mongo/db/audit.h"
#include "mongo/db/background.h"
#include "mongo/db/catalog/collection.h"
#include "mongo/db/clientcursor.h"
#include "mongo/db/concurrency/write_conflict_exception.h"
#include "mongo/db/curop.h"
#include "mongo/db/query/internal_plans.h"
#include "mongo/db/repl/oplog.h"
#include "mongo/db/repl/replication_coordinator_global.h"
#include "mongo/db/operation_context.h"
#include "mongo/util/log.h"
#include "mongo/util/processinfo.h"
#include "mongo/util/progress_meter.h"

namespace mongo {

using boost::scoped_ptr;
using std::string;
using std::endl;

/**
 * On rollback sets MultiIndexBlock::_needToCleanup to true.
 */
class MultiIndexBlock::SetNeedToCleanupOnRollback : public RecoveryUnit::Change {
public:
    explicit SetNeedToCleanupOnRollback(MultiIndexBlock* indexer) : _indexer(indexer) {}

    virtual void commit() {}
    virtual void rollback() {
        _indexer->_needToCleanup = true;
    }

private:
    MultiIndexBlock* const _indexer;
};

/**
 * On rollback in init(), cleans up _indexes so that ~MultiIndexBlock doesn't try to clean
 * up _indexes manually (since the changes were already rolled back).
 * Due to this, it is thus legal to call init() again after it fails.
 */
class MultiIndexBlock::CleanupIndexesVectorOnRollback : public RecoveryUnit::Change {
public:
    explicit CleanupIndexesVectorOnRollback(MultiIndexBlock* indexer) : _indexer(indexer) {}

    virtual void commit() {}
    virtual void rollback() {
        _indexer->_indexes.clear();
    }

private:
    MultiIndexBlock* const _indexer;
};

MultiIndexBlock::MultiIndexBlock(OperationContext* txn, Collection* collection)
    : _collection(collection),
      _txn(txn),
      _buildInBackground(false),
      _allowInterruption(false),
      _ignoreUnique(false),
      _needToCleanup(true) {}

MultiIndexBlock::~MultiIndexBlock() {
    if (!_needToCleanup || _indexes.empty())
        return;
    while (true) {
        try {
            WriteUnitOfWork wunit(_txn);
            // This cleans up all index builds.
            // Because that may need to write, it is done inside
            // of a WUOW. Nothing inside this block can fail, and it is made fatal if it does.
            for (size_t i = 0; i < _indexes.size(); i++) {
                _indexes[i].block->fail();
            }
            wunit.commit();
            return;
        } catch (const WriteConflictException& e) {
            continue;
        } catch (const std::exception& e) {
            error() << "Caught exception while cleaning up partially built indexes: " << e.what();
        } catch (...) {
            error() << "Caught unknown exception while cleaning up partially built indexes.";
        }
        fassertFailed(18644);
    }
}

void MultiIndexBlock::removeExistingIndexes(std::vector<BSONObj>* specs) const {
    for (size_t i = 0; i < specs->size(); i++) {
        Status status =
            _collection->getIndexCatalog()->prepareSpecForCreate(_txn, (*specs)[i]).getStatus();
        if (status.code() == ErrorCodes::IndexAlreadyExists) {
            specs->erase(specs->begin() + i);
            i--;
        }
        // intentionally ignoring other error codes
    }
}

Status MultiIndexBlock::init(const std::vector<BSONObj>& indexSpecs) {
    WriteUnitOfWork wunit(_txn);

    invariant(_indexes.empty());
    _txn->recoveryUnit()->registerChange(new CleanupIndexesVectorOnRollback(this));

    const string& ns = _collection->ns().ns();

    Status status = _collection->getIndexCatalog()->checkUnfinished();
    if (!status.isOK())
        return status;

    for (size_t i = 0; i < indexSpecs.size(); i++) {
        BSONObj info = indexSpecs[i];

        string pluginName = IndexNames::findPluginName(info["key"].Obj());
        if (pluginName.size()) {
            Status s = _collection->getIndexCatalog()->_upgradeDatabaseMinorVersionIfNeeded(
                _txn, pluginName);
            if (!s.isOK())
                return s;
        }

        // Any foreground indexes make all indexes be built in the foreground.
        _buildInBackground = (_buildInBackground && info["background"].trueValue());
    }

    for (size_t i = 0; i < indexSpecs.size(); i++) {
        BSONObj info = indexSpecs[i];
        StatusWith<BSONObj> statusWithInfo =
            _collection->getIndexCatalog()->prepareSpecForCreate(_txn, info);
        Status status = statusWithInfo.getStatus();
        if (!status.isOK())
            return status;
        info = statusWithInfo.getValue();

        IndexToBuild index;
        index.block = boost::make_shared<IndexCatalog::IndexBuildBlock>(_txn, _collection, info);
        status = index.block->init();
        if (!status.isOK())
            return status;

        index.real = index.block->getEntry()->accessMethod();
        status = index.real->initializeAsEmpty(_txn);
        if (!status.isOK())
            return status;

        if (!_buildInBackground) {
            // Bulk build process requires foreground building as it assumes nothing is changing
            // under it.
            index.bulk.reset(index.real->initiateBulk(_txn));
        }

        const IndexDescriptor* descriptor = index.block->getEntry()->descriptor();

        index.options.logIfError = false;  // logging happens elsewhere if needed.
        index.options.dupsAllowed = !descriptor->unique() || _ignoreUnique ||
            repl::getGlobalReplicationCoordinator()->shouldIgnoreUniqueIndex(descriptor);

        log() << "build index on: " << ns << " properties: " << descriptor->toString();
        if (index.bulk)
            log() << "\t building index using bulk method";

        // TODO SERVER-14888 Suppress this in cases we don't want to audit.
        audit::logCreateIndex(_txn->getClient(), &info, descriptor->indexName(), ns);

        _indexes.push_back(index);
    }

    // this is so that operations examining the list of indexes know there are more keys to look
    // at when doing things like in place updates, etc...
    _collection->infoCache()->addedIndex(_txn);

    if (_buildInBackground)
        _backgroundOperation.reset(new BackgroundOperation(ns));

    wunit.commit();
    return Status::OK();
}

Status MultiIndexBlock::insertAllDocumentsInCollection(std::set<RecordId>* dupsOut) {
    const char* curopMessage = _buildInBackground ? "Index Build (background)" : "Index Build";
    ProgressMeterHolder progress(
        *_txn->setMessage(curopMessage, curopMessage, _collection->numRecords(_txn)));

    Timer t;

    unsigned long long n = 0;

    scoped_ptr<PlanExecutor> exec(
        InternalPlanner::collectionScan(_txn, _collection->ns().ns(), _collection));
    if (_buildInBackground) {
        invariant(_allowInterruption);
        exec->setYieldPolicy(PlanExecutor::YIELD_AUTO);
    }

    Snapshotted<BSONObj> objToIndex;
    RecordId loc;
    PlanExecutor::ExecState state;
    int retries = 0;  // non-zero when retrying our last document.
    while (retries ||
           (PlanExecutor::ADVANCED == (state = exec->getNextSnapshotted(&objToIndex, &loc)))) {
        try {
            if (_allowInterruption)
                _txn->checkForInterrupt();

            // Make sure we are working with the latest version of the document.
            if (objToIndex.snapshotId() != _txn->recoveryUnit()->getSnapshotId() &&
                !_collection->findDoc(_txn, loc, &objToIndex)) {
                // doc was deleted so don't index it.
                retries = 0;
                continue;
            }

            // Done before insert so we can retry document if it WCEs.
            progress->setTotalWhileRunning(_collection->numRecords(_txn));

            WriteUnitOfWork wunit(_txn);
            Status ret = insert(objToIndex.value(), loc);
            if (ret.isOK()) {
                wunit.commit();
            } else if (dupsOut && ret.code() == ErrorCodes::DuplicateKey) {
                // If dupsOut is non-null, we should only fail the specific insert that
                // led to a DuplicateKey rather than the whole index build.
                dupsOut->insert(loc);
            } else {
                // Fail the index build hard.
                return ret;
            }

            // Go to the next document
            progress->hit();
            n++;
            retries = 0;
        } catch (const WriteConflictException& wce) {
            _txn->getCurOp()->debug().writeConflicts++;
            retries++;  // logAndBackoff expects this to be 1 on first call.
            wce.logAndBackoff(retries, "index creation", _collection->ns().ns());

            // Can't use WRITE_CONFLICT_RETRY_LOOP macros since we need to save/restore exec
            // around call to commitAndRestart.
            exec->saveState();
            _txn->recoveryUnit()->commitAndRestart();
            exec->restoreState(_txn);  // Handles any WCEs internally.
        }
    }

    if (state != PlanExecutor::IS_EOF) {
        // If the plan executor was killed, this means the DB/collection was dropped and so it
        // is not safe to cleanup the in-progress indexes.
        if (state == PlanExecutor::DEAD) {
            abortWithoutCleanup();
        }

        uasserted(28550, "Unable to complete index build as the collection is no longer readable");
    }

    progress->finished();

    Status ret = doneInserting(dupsOut);
    if (!ret.isOK())
        return ret;

    log() << "build index done.  scanned " << n << " total records. " << t.seconds() << " secs"
          << endl;

    return Status::OK();
}

Status MultiIndexBlock::insert(const BSONObj& doc, const RecordId& loc) {
    for (size_t i = 0; i < _indexes.size(); i++) {
        int64_t unused;
        Status idxStatus =
            _indexes[i].forInsert()->insert(_txn, doc, loc, _indexes[i].options, &unused);
        if (!idxStatus.isOK())
            return idxStatus;
    }
    return Status::OK();
}

Status MultiIndexBlock::doneInserting(std::set<RecordId>* dupsOut) {
    for (size_t i = 0; i < _indexes.size(); i++) {
        if (_indexes[i].bulk == NULL)
            continue;
        LOG(1) << "\t bulk commit starting for index: "
               << _indexes[i].block->getEntry()->descriptor()->indexName();
        Status status = _indexes[i].real->commitBulk(
            _indexes[i].bulk.get(), _allowInterruption, _indexes[i].options.dupsAllowed, dupsOut);
        if (!status.isOK()) {
            return status;
        }
    }

    return Status::OK();
}

void MultiIndexBlock::abortWithoutCleanup() {
    _indexes.clear();
    _needToCleanup = false;
}

void MultiIndexBlock::commit() {
    for (size_t i = 0; i < _indexes.size(); i++) {
        _indexes[i].block->success();
    }

    // this one is so operations examining the list of indexes know that the index is finished
    _collection->infoCache()->addedIndex(_txn);

    _txn->recoveryUnit()->registerChange(new SetNeedToCleanupOnRollback(this));
    _needToCleanup = false;
}

}  // namespace mongo
