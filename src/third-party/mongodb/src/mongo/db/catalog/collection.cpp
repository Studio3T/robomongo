// collection.cpp

/**
*    Copyright (C) 2013-2014 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kStorage

#include "mongo/platform/basic.h"

#include "mongo/db/catalog/collection.h"

#include <boost/scoped_ptr.hpp>

#include "mongo/base/counter.h"
#include "mongo/base/owned_pointer_map.h"
#include "mongo/db/clientcursor.h"
#include "mongo/db/commands/server_status_metric.h"
#include "mongo/db/curop.h"
#include "mongo/db/catalog/collection_catalog_entry.h"
#include "mongo/db/catalog/database_catalog_entry.h"
#include "mongo/db/catalog/index_create.h"
#include "mongo/db/index/index_access_method.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/repl/replication_coordinator_global.h"
#include "mongo/db/storage/mmap_v1/mmap_v1_options.h"
#include "mongo/db/storage/record_fetcher.h"

#include "mongo/db/auth/user_document_parser.h"  // XXX-ANDY
#include "mongo/util/log.h"

namespace mongo {

using boost::scoped_ptr;
using std::endl;
using std::string;
using std::vector;

using logger::LogComponent;

std::string CompactOptions::toString() const {
    std::stringstream ss;
    ss << "paddingMode: ";
    switch (paddingMode) {
        case NONE:
            ss << "NONE";
            break;
        case PRESERVE:
            ss << "PRESERVE";
            break;
        case MANUAL:
            ss << "MANUAL (" << paddingBytes << " + ( doc * " << paddingFactor << ") )";
    }

    ss << " validateDocuments: " << validateDocuments;

    return ss.str();
}

// ----

Collection::Collection(OperationContext* txn,
                       const StringData& fullNS,
                       CollectionCatalogEntry* details,
                       RecordStore* recordStore,
                       DatabaseCatalogEntry* dbce)
    : _ns(fullNS),
      _details(details),
      _recordStore(recordStore),
      _dbce(dbce),
      _infoCache(this),
      _indexCatalog(this),
      _cursorManager(fullNS) {
    _magic = 1357924;
    _indexCatalog.init(txn);
    if (isCapped())
        _recordStore->setCappedDeleteCallback(this);
    _infoCache.reset(txn);
}

Collection::~Collection() {
    verify(ok());
    _magic = 0;
}

bool Collection::requiresIdIndex() const {
    if (_ns.ns().find('$') != string::npos) {
        // no indexes on indexes
        return false;
    }

    if (_ns.isSystem()) {
        StringData shortName = _ns.coll().substr(_ns.coll().find('.') + 1);
        if (shortName == "indexes" || shortName == "namespaces" || shortName == "profile") {
            return false;
        }
    }

    if (_ns.db() == "local") {
        if (_ns.coll().startsWith("oplog."))
            return false;
    }

    if (!_ns.isSystem()) {
        // non system collections definitely have an _id index
        return true;
    }


    return true;
}

RecordIterator* Collection::getIterator(OperationContext* txn,
                                        const RecordId& start,
                                        const CollectionScanParams::Direction& dir) const {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IS));
    invariant(ok());

    return _recordStore->getIterator(txn, start, dir);
}

vector<RecordIterator*> Collection::getManyIterators(OperationContext* txn) const {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IS));

    return _recordStore->getManyIterators(txn);
}

Snapshotted<BSONObj> Collection::docFor(OperationContext* txn, const RecordId& loc) const {
    return Snapshotted<BSONObj>(txn->recoveryUnit()->getSnapshotId(),
                                _recordStore->dataFor(txn, loc).releaseToBson());
}

bool Collection::findDoc(OperationContext* txn,
                         const RecordId& loc,
                         Snapshotted<BSONObj>* out) const {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IS));

    RecordData rd;
    if (!_recordStore->findRecord(txn, loc, &rd))
        return false;
    *out = Snapshotted<BSONObj>(txn->recoveryUnit()->getSnapshotId(), rd.releaseToBson());
    return true;
}

StatusWith<RecordId> Collection::insertDocument(OperationContext* txn,
                                                const DocWriter* doc,
                                                bool enforceQuota) {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IX));
    invariant(!_indexCatalog.haveAnyIndexes());  // eventually can implement, just not done

    StatusWith<RecordId> loc = _recordStore->insertRecord(txn, doc, _enforceQuota(enforceQuota));
    if (!loc.isOK())
        return loc;

    return StatusWith<RecordId>(loc);
}

StatusWith<RecordId> Collection::insertDocument(OperationContext* txn,
                                                const BSONObj& docToInsert,
                                                bool enforceQuota) {
    const SnapshotId sid = txn->recoveryUnit()->getSnapshotId();

    if (_indexCatalog.findIdIndex(txn)) {
        if (docToInsert["_id"].eoo()) {
            return StatusWith<RecordId>(ErrorCodes::InternalError,
                                        str::stream()
                                            << "Collection::insertDocument got "
                                               "document without _id for ns:" << _ns.ns());
        }
    }

    StatusWith<RecordId> res = _insertDocument(txn, docToInsert, enforceQuota);
    invariant(sid == txn->recoveryUnit()->getSnapshotId());
    return res;
}

StatusWith<RecordId> Collection::insertDocument(OperationContext* txn,
                                                const BSONObj& doc,
                                                MultiIndexBlock* indexBlock,
                                                bool enforceQuota) {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IX));

    StatusWith<RecordId> loc =
        _recordStore->insertRecord(txn, doc.objdata(), doc.objsize(), _enforceQuota(enforceQuota));

    if (!loc.isOK())
        return loc;

    Status status = indexBlock->insert(doc, loc.getValue());
    if (!status.isOK())
        return StatusWith<RecordId>(status);

    return loc;
}

RecordFetcher* Collection::documentNeedsFetch(OperationContext* txn, const RecordId& loc) const {
    return _recordStore->recordNeedsFetch(txn, loc);
}


StatusWith<RecordId> Collection::_insertDocument(OperationContext* txn,
                                                 const BSONObj& docToInsert,
                                                 bool enforceQuota) {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IX));

    // TODO: for now, capped logic lives inside NamespaceDetails, which is hidden
    //       under the RecordStore, this feels broken since that should be a
    //       collection access method probably

    StatusWith<RecordId> loc = _recordStore->insertRecord(
        txn, docToInsert.objdata(), docToInsert.objsize(), _enforceQuota(enforceQuota));
    if (!loc.isOK())
        return loc;

    invariant(RecordId::min() < loc.getValue());
    invariant(loc.getValue() < RecordId::max());

    _infoCache.notifyOfWriteOp();

    Status s = _indexCatalog.indexRecord(txn, docToInsert, loc.getValue());
    if (!s.isOK())
        return StatusWith<RecordId>(s);

    return loc;
}

Status Collection::aboutToDeleteCapped(OperationContext* txn,
                                       const RecordId& loc,
                                       RecordData data) {
    /* check if any cursors point to us.  if so, advance them. */
    _cursorManager.invalidateDocument(txn, loc, INVALIDATION_DELETION);

    BSONObj doc = data.releaseToBson();
    _indexCatalog.unindexRecord(txn, doc, loc, false);

    return Status::OK();
}

void Collection::deleteDocument(
    OperationContext* txn, const RecordId& loc, bool cappedOK, bool noWarn, BSONObj* deletedId) {
    if (isCapped() && !cappedOK) {
        log() << "failing remove on a capped ns " << _ns << endl;
        uasserted(10089, "cannot remove from a capped collection");
        return;
    }

    Snapshotted<BSONObj> doc = docFor(txn, loc);

    if (deletedId) {
        BSONElement e = doc.value()["_id"];
        if (e.type()) {
            *deletedId = e.wrap();
        }
    }

    /* check if any cursors point to us.  if so, advance them. */
    _cursorManager.invalidateDocument(txn, loc, INVALIDATION_DELETION);

    _indexCatalog.unindexRecord(txn, doc.value(), loc, noWarn);

    _recordStore->deleteRecord(txn, loc);

    _infoCache.notifyOfWriteOp();
}

Counter64 moveCounter;
ServerStatusMetricField<Counter64> moveCounterDisplay("record.moves", &moveCounter);

StatusWith<RecordId> Collection::updateDocument(OperationContext* txn,
                                                const RecordId& oldLocation,
                                                const Snapshotted<BSONObj>& objOld,
                                                const BSONObj& objNew,
                                                bool enforceQuota,
                                                bool indexesAffected,
                                                OpDebug* debug) {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IX));
    invariant(objOld.snapshotId() == txn->recoveryUnit()->getSnapshotId());

    SnapshotId sid = txn->recoveryUnit()->getSnapshotId();

    BSONElement oldId = objOld.value()["_id"];
    if (!oldId.eoo() && (oldId != objNew["_id"]))
        return StatusWith<RecordId>(
            ErrorCodes::InternalError, "in Collection::updateDocument _id mismatch", 13596);

    // At the end of this step, we will have a map of UpdateTickets, one per index, which
    // represent the index updates needed to be done, based on the changes between objOld and
    // objNew.
    OwnedPointerMap<IndexDescriptor*, UpdateTicket> updateTickets;
    if (indexesAffected) {
        IndexCatalog::IndexIterator ii = _indexCatalog.getIndexIterator(txn, true);
        while (ii.more()) {
            IndexDescriptor* descriptor = ii.next();
            IndexAccessMethod* iam = _indexCatalog.getIndex(descriptor);

            InsertDeleteOptions options;
            options.logIfError = false;
            options.dupsAllowed =
                !(KeyPattern::isIdKeyPattern(descriptor->keyPattern()) || descriptor->unique()) ||
                repl::getGlobalReplicationCoordinator()->shouldIgnoreUniqueIndex(descriptor);
            UpdateTicket* updateTicket = new UpdateTicket();
            updateTickets.mutableMap()[descriptor] = updateTicket;
            Status ret = iam->validateUpdate(
                txn, objOld.value(), objNew, oldLocation, options, updateTicket);
            if (!ret.isOK()) {
                return StatusWith<RecordId>(ret);
            }
        }
    }

    // This can call back into Collection::recordStoreGoingToMove.  If that happens, the old
    // object is removed from all indexes.
    StatusWith<RecordId> newLocation = _recordStore->updateRecord(
        txn, oldLocation, objNew.objdata(), objNew.objsize(), _enforceQuota(enforceQuota), this);

    if (!newLocation.isOK()) {
        return newLocation;
    }

    // At this point, the old object may or may not still be indexed, depending on if it was
    // moved.

    _infoCache.notifyOfWriteOp();

    // If the object did move, we need to add the new location to all indexes.
    if (newLocation.getValue() != oldLocation) {
        if (debug) {
            if (debug->nmoved == -1)  // default of -1 rather than 0
                debug->nmoved = 1;
            else
                debug->nmoved += 1;
        }

        Status s = _indexCatalog.indexRecord(txn, objNew, newLocation.getValue());
        if (!s.isOK())
            return StatusWith<RecordId>(s);
        invariant(sid == txn->recoveryUnit()->getSnapshotId());
        return newLocation;
    }

    // Object did not move.  We update each index with each respective UpdateTicket.

    if (debug)
        debug->keyUpdates = 0;

    if (indexesAffected) {
        IndexCatalog::IndexIterator ii = _indexCatalog.getIndexIterator(txn, true);
        while (ii.more()) {
            IndexDescriptor* descriptor = ii.next();
            IndexAccessMethod* iam = _indexCatalog.getIndex(descriptor);

            int64_t updatedKeys;
            Status ret = iam->update(txn, *updateTickets.mutableMap()[descriptor], &updatedKeys);
            if (!ret.isOK())
                return StatusWith<RecordId>(ret);
            if (debug)
                debug->keyUpdates += updatedKeys;
        }
    }

    invariant(sid == txn->recoveryUnit()->getSnapshotId());
    return newLocation;
}

Status Collection::recordStoreGoingToMove(OperationContext* txn,
                                          const RecordId& oldLocation,
                                          const char* oldBuffer,
                                          size_t oldSize) {
    moveCounter.increment();
    _cursorManager.invalidateDocument(txn, oldLocation, INVALIDATION_DELETION);
    _indexCatalog.unindexRecord(txn, BSONObj(oldBuffer), oldLocation, true);
    return Status::OK();
}

Status Collection::recordStoreGoingToUpdateInPlace(OperationContext* txn, const RecordId& loc) {
    // Broadcast the mutation so that query results stay correct.
    _cursorManager.invalidateDocument(txn, loc, INVALIDATION_MUTATION);
    return Status::OK();
}


Status Collection::updateDocumentWithDamages(OperationContext* txn,
                                             const RecordId& loc,
                                             const Snapshotted<RecordData>& oldRec,
                                             const char* damageSource,
                                             const mutablebson::DamageVector& damages) {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IX));
    invariant(oldRec.snapshotId() == txn->recoveryUnit()->getSnapshotId());

    // Broadcast the mutation so that query results stay correct.
    _cursorManager.invalidateDocument(txn, loc, INVALIDATION_MUTATION);

    return _recordStore->updateWithDamages(txn, loc, oldRec.value(), damageSource, damages);
}

bool Collection::_enforceQuota(bool userEnforeQuota) const {
    if (!userEnforeQuota)
        return false;

    if (!mmapv1GlobalOptions.quota)
        return false;

    if (_ns.db() == "local")
        return false;

    if (_ns.isSpecial())
        return false;

    return true;
}

bool Collection::isCapped() const {
    return _recordStore->isCapped();
}

uint64_t Collection::numRecords(OperationContext* txn) const {
    return _recordStore->numRecords(txn);
}

uint64_t Collection::dataSize(OperationContext* txn) const {
    return _recordStore->dataSize(txn);
}

uint64_t Collection::getIndexSize(OperationContext* opCtx, BSONObjBuilder* details, int scale) {
    IndexCatalog* idxCatalog = getIndexCatalog();

    IndexCatalog::IndexIterator ii = idxCatalog->getIndexIterator(opCtx, true);

    uint64_t totalSize = 0;

    while (ii.more()) {
        IndexDescriptor* d = ii.next();
        IndexAccessMethod* iam = idxCatalog->getIndex(d);

        long long ds = iam->getSpaceUsedBytes(opCtx);

        totalSize += ds;
        if (details) {
            details->appendNumber(d->indexName(), ds / scale);
        }
    }

    return totalSize;
}

/**
 * order will be:
 * 1) store index specs
 * 2) drop indexes
 * 3) truncate record store
 * 4) re-write indexes
 */
Status Collection::truncate(OperationContext* txn) {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_X));
    massert(17445, "index build in progress", _indexCatalog.numIndexesInProgress(txn) == 0);

    // 1) store index specs
    vector<BSONObj> indexSpecs;
    {
        IndexCatalog::IndexIterator ii = _indexCatalog.getIndexIterator(txn, false);
        while (ii.more()) {
            const IndexDescriptor* idx = ii.next();
            indexSpecs.push_back(idx->infoObj().getOwned());
        }
    }

    // 2) drop indexes
    Status status = _indexCatalog.dropAllIndexes(txn, true);
    if (!status.isOK())
        return status;
    _cursorManager.invalidateAll(false);
    _infoCache.reset(txn);

    // 3) truncate record store
    status = _recordStore->truncate(txn);
    if (!status.isOK())
        return status;

    // 4) re-create indexes
    for (size_t i = 0; i < indexSpecs.size(); i++) {
        status = _indexCatalog.createIndexOnEmptyCollection(txn, indexSpecs[i]);
        if (!status.isOK())
            return status;
    }

    return Status::OK();
}

void Collection::temp_cappedTruncateAfter(OperationContext* txn, RecordId end, bool inclusive) {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IX));
    invariant(isCapped());

    _cursorManager.invalidateAll(false);
    _recordStore->temp_cappedTruncateAfter(txn, end, inclusive);
}

namespace {
class MyValidateAdaptor : public ValidateAdaptor {
public:
    virtual ~MyValidateAdaptor() {}

    virtual Status validate(const RecordData& record, size_t* dataSize) {
        BSONObj obj = record.toBson();
        const Status status = validateBSON(obj.objdata(), obj.objsize());
        if (status.isOK())
            *dataSize = obj.objsize();
        return Status::OK();
    }
};
}

Status Collection::validate(OperationContext* txn,
                            bool full,
                            bool scanData,
                            ValidateResults* results,
                            BSONObjBuilder* output) {
    dassert(txn->lockState()->isCollectionLockedForMode(ns().toString(), MODE_IS));

    MyValidateAdaptor adaptor;
    Status status = _recordStore->validate(txn, full, scanData, &adaptor, results, output);
    if (!status.isOK())
        return status;

    {  // indexes
        output->append("nIndexes", _indexCatalog.numIndexesReady(txn));
        int idxn = 0;
        try {
            // Only applicable when 'full' validation is requested.
            boost::scoped_ptr<BSONObjBuilder> indexDetails(full ? new BSONObjBuilder() : NULL);
            BSONObjBuilder indexes;  // not using subObjStart to be exception safe

            IndexCatalog::IndexIterator i = _indexCatalog.getIndexIterator(txn, false);
            while (i.more()) {
                const IndexDescriptor* descriptor = i.next();
                log(LogComponent::kIndex) << "validating index " << descriptor->indexNamespace()
                                          << endl;
                IndexAccessMethod* iam = _indexCatalog.getIndex(descriptor);
                invariant(iam);

                boost::scoped_ptr<BSONObjBuilder> bob(
                    indexDetails.get() ? new BSONObjBuilder(indexDetails->subobjStart(
                                             descriptor->indexNamespace()))
                                       : NULL);

                int64_t keys;
                iam->validate(txn, full, &keys, bob.get());
                indexes.appendNumber(descriptor->indexNamespace(), static_cast<long long>(keys));

                if (bob) {
                    BSONObj obj = bob->done();
                    BSONElement valid = obj["valid"];
                    if (valid.ok() && !valid.trueValue()) {
                        results->valid = false;
                    }
                }
                idxn++;
            }

            output->append("keysPerIndex", indexes.done());
            if (indexDetails.get()) {
                output->append("indexDetails", indexDetails->done());
            }
        } catch (DBException& exc) {
            string err = str::stream() << "exception during index validate idxn "
                                       << BSONObjBuilder::numStr(idxn) << ": " << exc.toString();
            results->errors.push_back(err);
            results->valid = false;
        }
    }

    return Status::OK();
}

Status Collection::touch(OperationContext* txn,
                         bool touchData,
                         bool touchIndexes,
                         BSONObjBuilder* output) const {
    if (touchData) {
        BSONObjBuilder b;
        Status status = _recordStore->touch(txn, &b);
        if (!status.isOK())
            return status;
        output->append("data", b.obj());
    }

    if (touchIndexes) {
        Timer t;
        IndexCatalog::IndexIterator ii = _indexCatalog.getIndexIterator(txn, false);
        while (ii.more()) {
            const IndexDescriptor* desc = ii.next();
            const IndexAccessMethod* iam = _indexCatalog.getIndex(desc);
            Status status = iam->touch(txn);
            if (!status.isOK())
                return status;
        }

        output->append("indexes",
                       BSON("num" << _indexCatalog.numIndexesTotal(txn) << "millis" << t.millis()));
    }

    return Status::OK();
}
}
