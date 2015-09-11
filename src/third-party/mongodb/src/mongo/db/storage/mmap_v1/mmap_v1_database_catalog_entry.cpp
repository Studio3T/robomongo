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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kStorage

#include "mongo/platform/basic.h"

#include "mongo/db/storage/mmap_v1/mmap_v1_database_catalog_entry.h"

#include <boost/scoped_ptr.hpp>
#include <utility>

#include "mongo/db/catalog/index_catalog_entry.h"
#include "mongo/db/index/2d_access_method.h"
#include "mongo/db/index/btree_access_method.h"
#include "mongo/db/index/btree_based_access_method.h"
#include "mongo/db/index/fts_access_method.h"
#include "mongo/db/index/hash_access_method.h"
#include "mongo/db/index/haystack_access_method.h"
#include "mongo/db/index/s2_access_method.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/server_parameters.h"
#include "mongo/db/storage/mmap_v1/btree/btree_interface.h"
#include "mongo/db/storage/mmap_v1/catalog/namespace_details.h"
#include "mongo/db/storage/mmap_v1/catalog/namespace_details_collection_entry.h"
#include "mongo/db/storage/mmap_v1/catalog/namespace_details_rsv1_metadata.h"
#include "mongo/db/storage/mmap_v1/data_file.h"
#include "mongo/db/storage/mmap_v1/record_store_v1_capped.h"
#include "mongo/db/storage/mmap_v1/record_store_v1_simple.h"
#include "mongo/util/log.h"

namespace mongo {

using boost::scoped_ptr;
using std::auto_ptr;

namespace {

/**
 * Declaration for the "newCollectionsUsePowerOf2Sizes" server parameter, which is now
 * deprecated in 3.0.
 * Note that:
 * - setting to true performs a no-op.
 * - setting to false will fail.
 */
class NewCollectionsUsePowerOf2SizesParameter : public ExportedServerParameter<bool> {
public:
    NewCollectionsUsePowerOf2SizesParameter()
        : ExportedServerParameter<bool>(ServerParameterSet::getGlobal(),
                                        "newCollectionsUsePowerOf2Sizes",
                                        &newCollectionsUsePowerOf2SizesFlag,
                                        true,
                                        true),
          newCollectionsUsePowerOf2SizesFlag(true) {}

    virtual Status validate(const bool& potentialNewValue) {
        if (!potentialNewValue) {
            return Status(ErrorCodes::BadValue,
                          "newCollectionsUsePowerOf2Sizes cannot be set to false. "
                          "Use noPadding instead during createCollection.");
        }

        return Status::OK();
    }

private:
    // Unused, needed for server parameter.
    bool newCollectionsUsePowerOf2SizesFlag;

} exportedNewCollectionsUsePowerOf2SizesParameter;


int _massageExtentSize(const ExtentManager* em, long long size) {
    if (size < em->minSize())
        return em->minSize();
    if (size > em->maxSize())
        return em->maxSize();

    return static_cast<int>(size);
}

}  // namespace


/**
 * Registers the insertion of a new entry in the _collections cache with the RecoveryUnit,
 * allowing for rollback.
 */
class MMAPV1DatabaseCatalogEntry::EntryInsertion : public RecoveryUnit::Change {
public:
    EntryInsertion(const StringData& ns, MMAPV1DatabaseCatalogEntry* entry)
        : _ns(ns.toString()), _entry(entry) {}

    void rollback() {
        _entry->_removeFromCache(NULL, _ns);
    }

    void commit() {}

private:
    const std::string _ns;
    MMAPV1DatabaseCatalogEntry* const _entry;
};

/**
 * Registers the removal of an entry from the _collections cache with the RecoveryUnit,
 * delaying actual deletion of the information until the change is commited. This allows
 * for easy rollback.
 */
class MMAPV1DatabaseCatalogEntry::EntryRemoval : public RecoveryUnit::Change {
public:
    //  Rollback removing the collection from the cache. Takes ownership of the cachedEntry,
    //  and will delete it if removal is final.
    EntryRemoval(const StringData& ns, MMAPV1DatabaseCatalogEntry* catalogEntry, Entry* cachedEntry)
        : _ns(ns.toString()), _catalogEntry(catalogEntry), _cachedEntry(cachedEntry) {}

    void rollback() {
        _catalogEntry->_collections[_ns] = _cachedEntry;
    }

    void commit() {
        delete _cachedEntry;
    }

private:
    const std::string _ns;
    MMAPV1DatabaseCatalogEntry* const _catalogEntry;
    Entry* const _cachedEntry;
};

MMAPV1DatabaseCatalogEntry::MMAPV1DatabaseCatalogEntry(OperationContext* txn,
                                                       const StringData& name,
                                                       const StringData& path,
                                                       bool directoryPerDB,
                                                       bool transient)
    : DatabaseCatalogEntry(name),
      _path(path.toString()),
      _namespaceIndex(_path, name.toString()),
      _extentManager(name, path, directoryPerDB) {
    invariant(txn->lockState()->isDbLockedForMode(name, MODE_X));

    try {
        // First init the .ns file. If this fails, we may leak the .ns file, but this is OK
        // because subsequent openDB will go through this code path again.
        _namespaceIndex.init(txn);

        // Initialize the extent manager. This will create the first data file (.0) if needed
        // and if this fails we would leak the .ns file above. Leaking the .ns or .0 file is
        // acceptable, because subsequent openDB calls will exercise the code path again.
        Status s = _extentManager.init(txn);
        if (!s.isOK()) {
            msgasserted(16966, str::stream() << "_extentManager.init failed: " << s.toString());
        }

        // This is the actual loading of the on-disk structures into cache.
        _init(txn);
    } catch (const DBException& dbe) {
        warning() << "database " << path << " " << name
                  << " could not be opened due to DBException " << dbe.getCode() << ": "
                  << dbe.what();
        throw;
    } catch (const std::exception& e) {
        warning() << "database " << path << " " << name << " could not be opened " << e.what();
        throw;
    }
}

MMAPV1DatabaseCatalogEntry::~MMAPV1DatabaseCatalogEntry() {
    for (CollectionMap::const_iterator i = _collections.begin(); i != _collections.end(); ++i) {
        delete i->second;
    }
    _collections.clear();
}

intmax_t dbSize(const string& database);  // from repair_database.cpp

int64_t MMAPV1DatabaseCatalogEntry::sizeOnDisk(OperationContext* opCtx) const {
    return static_cast<int64_t>(dbSize(name()));
}

void MMAPV1DatabaseCatalogEntry::_removeFromCache(RecoveryUnit* ru, const StringData& ns) {
    CollectionMap::iterator i = _collections.find(ns.toString());
    if (i == _collections.end()) {
        return;
    }

    //  If there is an operation context, register a rollback to restore the cache entry
    if (ru) {
        ru->registerChange(new EntryRemoval(ns, this, i->second));
    } else {
        delete i->second;
    }
    _collections.erase(i);
}

Status MMAPV1DatabaseCatalogEntry::dropCollection(OperationContext* txn, const StringData& ns) {
    invariant(txn->lockState()->isCollectionLockedForMode(ns, MODE_X));
    _removeFromCache(txn->recoveryUnit(), ns);

    NamespaceDetails* details = _namespaceIndex.details(ns);

    if (!details) {
        return Status(ErrorCodes::NamespaceNotFound, str::stream() << "ns not found: " << ns);
    }

    invariant(details->nIndexes == 0);               // TODO: delete instead?
    invariant(details->indexBuildsInProgress == 0);  // TODO: delete instead?

    _removeNamespaceFromNamespaceCollection(txn, ns);

    // free extents
    if (!details->firstExtent.isNull()) {
        _extentManager.freeExtents(txn, details->firstExtent, details->lastExtent);
        *txn->recoveryUnit()->writing(&details->firstExtent) = DiskLoc().setInvalid();
        *txn->recoveryUnit()->writing(&details->lastExtent) = DiskLoc().setInvalid();
    }

    // remove from the catalog hashtable
    _namespaceIndex.kill_ns(txn, ns);

    return Status::OK();
}


Status MMAPV1DatabaseCatalogEntry::renameCollection(OperationContext* txn,
                                                    const StringData& fromNS,
                                                    const StringData& toNS,
                                                    bool stayTemp) {
    Status s = _renameSingleNamespace(txn, fromNS, toNS, stayTemp);
    if (!s.isOK())
        return s;

    NamespaceDetails* details = _namespaceIndex.details(toNS);
    invariant(details);

    RecordStoreV1Base* systemIndexRecordStore = _getIndexRecordStore();
    scoped_ptr<RecordIterator> it(systemIndexRecordStore->getIterator(txn));

    while (!it->isEOF()) {
        RecordId loc = it->getNext();
        BSONObj oldIndexSpec = it->dataFor(loc).toBson();
        if (fromNS != oldIndexSpec["ns"].valuestrsafe())
            continue;

        BSONObj newIndexSpec;
        {
            BSONObjBuilder b;
            BSONObjIterator i(oldIndexSpec);
            while (i.more()) {
                BSONElement e = i.next();
                if (strcmp(e.fieldName(), "ns") != 0)
                    b.append(e);
                else
                    b << "ns" << toNS;
            }
            newIndexSpec = b.obj();
        }

        StatusWith<RecordId> newIndexSpecLoc = systemIndexRecordStore->insertRecord(
            txn, newIndexSpec.objdata(), newIndexSpec.objsize(), false);
        if (!newIndexSpecLoc.isOK())
            return newIndexSpecLoc.getStatus();

        const string& indexName = oldIndexSpec.getStringField("name");

        {
            // fix IndexDetails pointer
            NamespaceDetailsCollectionCatalogEntry ce(
                toNS, details, _getNamespaceRecordStore(), systemIndexRecordStore, this);
            int indexI = ce._findIndexNumber(txn, indexName);

            IndexDetails& indexDetails = details->idx(indexI);
            *txn->recoveryUnit()->writing(&indexDetails.info) =
                DiskLoc::fromRecordId(newIndexSpecLoc.getValue());
        }

        {
            // move underlying namespac
            string oldIndexNs = IndexDescriptor::makeIndexNamespace(fromNS, indexName);
            string newIndexNs = IndexDescriptor::makeIndexNamespace(toNS, indexName);

            Status s = _renameSingleNamespace(txn, oldIndexNs, newIndexNs, false);
            if (!s.isOK())
                return s;
        }

        systemIndexRecordStore->deleteRecord(txn, loc);
    }

    return Status::OK();
}

Status MMAPV1DatabaseCatalogEntry::_renameSingleNamespace(OperationContext* txn,
                                                          const StringData& fromNS,
                                                          const StringData& toNS,
                                                          bool stayTemp) {
    // some sanity checking
    NamespaceDetails* fromDetails = _namespaceIndex.details(fromNS);
    if (!fromDetails)
        return Status(ErrorCodes::BadValue, "from namespace doesn't exist");

    if (_namespaceIndex.details(toNS))
        return Status(ErrorCodes::BadValue, "to namespace already exists");

    _removeFromCache(txn->recoveryUnit(), fromNS);

    // at this point, we haven't done anything destructive yet

    // ----
    // actually start moving
    // ----

    // this could throw, but if it does we're ok
    _namespaceIndex.add_ns(txn, toNS, fromDetails);
    NamespaceDetails* toDetails = _namespaceIndex.details(toNS);

    try {
        toDetails->copyingFrom(txn, toNS, _namespaceIndex, fromDetails);  // fixes extraOffset
    } catch (DBException&) {
        // could end up here if .ns is full - if so try to clean up / roll back a little
        _namespaceIndex.kill_ns(txn, toNS);
        throw;
    }

    // at this point, code .ns stuff moved

    _namespaceIndex.kill_ns(txn, fromNS);
    fromDetails = NULL;

    // fix system.namespaces
    BSONObj newSpec;
    RecordId oldSpecLocation;
    {
        BSONObj oldSpec;
        {
            RecordStoreV1Base* rs = _getNamespaceRecordStore();
            scoped_ptr<RecordIterator> it(rs->getIterator(txn));
            while (!it->isEOF()) {
                RecordId loc = it->getNext();
                BSONObj entry = it->dataFor(loc).toBson();
                if (fromNS == entry["name"].String()) {
                    oldSpecLocation = loc;
                    oldSpec = entry.getOwned();
                    break;
                }
            }
        }
        invariant(!oldSpec.isEmpty());
        invariant(!oldSpecLocation.isNull());

        BSONObjBuilder b;
        BSONObjIterator i(oldSpec.getObjectField("options"));
        while (i.more()) {
            BSONElement e = i.next();
            if (strcmp(e.fieldName(), "create") != 0) {
                if (stayTemp || (strcmp(e.fieldName(), "temp") != 0))
                    b.append(e);
            } else {
                b << "create" << toNS;
            }
        }
        newSpec = b.obj();
    }

    _addNamespaceToNamespaceCollection(txn, toNS, newSpec.isEmpty() ? 0 : &newSpec);

    _getNamespaceRecordStore()->deleteRecord(txn, oldSpecLocation);

    Entry*& entry = _collections[toNS.toString()];
    invariant(entry == NULL);
    txn->recoveryUnit()->registerChange(new EntryInsertion(toNS, this));
    entry = new Entry();
    _insertInCache(txn, toNS, entry);

    return Status::OK();
}

void MMAPV1DatabaseCatalogEntry::appendExtraStats(OperationContext* opCtx,
                                                  BSONObjBuilder* output,
                                                  double scale) const {
    if (isEmpty()) {
        output->appendNumber("fileSize", 0);
    } else {
        output->appendNumber("fileSize", _extentManager.fileSize() / scale);
        output->appendNumber("nsSizeMB",
                             static_cast<int>(_namespaceIndex.fileLength() / (1024 * 1024)));

        int freeListSize = 0;
        int64_t freeListSpace = 0;
        _extentManager.freeListStats(opCtx, &freeListSize, &freeListSpace);

        BSONObjBuilder extentFreeList(output->subobjStart("extentFreeList"));
        extentFreeList.append("num", freeListSize);
        extentFreeList.appendNumber("totalSize", static_cast<long long>(freeListSpace / scale));
        extentFreeList.done();

        {
            const DataFileVersion version = _extentManager.getFileFormat(opCtx);

            BSONObjBuilder dataFileVersion(output->subobjStart("dataFileVersion"));
            dataFileVersion.append("major", version.majorRaw());
            dataFileVersion.append("minor", version.minorRaw());
            dataFileVersion.done();
        }
    }
}

bool MMAPV1DatabaseCatalogEntry::isOlderThan24(OperationContext* opCtx) const {
    if (_extentManager.numFiles() == 0)
        return false;

    const DataFileVersion version = _extentManager.getFileFormat(opCtx);

    invariant(version.isCompatibleWithCurrentCode());

    return !version.is24IndexClean();
}

void MMAPV1DatabaseCatalogEntry::markIndexSafe24AndUp(OperationContext* opCtx) {
    if (_extentManager.numFiles() == 0)
        return;

    DataFileVersion version = _extentManager.getFileFormat(opCtx);

    invariant(version.isCompatibleWithCurrentCode());

    if (version.is24IndexClean())
        return;  // nothing to do

    version.setIs24IndexClean();
    _extentManager.setFileFormat(opCtx, version);
}

bool MMAPV1DatabaseCatalogEntry::currentFilesCompatible(OperationContext* opCtx) const {
    if (_extentManager.numFiles() == 0)
        return true;

    return _extentManager.getOpenFile(0)->getHeader()->version.isCompatibleWithCurrentCode();
}

void MMAPV1DatabaseCatalogEntry::getCollectionNamespaces(std::list<std::string>* tofill) const {
    _namespaceIndex.getCollectionNamespaces(tofill);
}

void MMAPV1DatabaseCatalogEntry::_ensureSystemCollection(OperationContext* txn,
                                                         const StringData& ns) {
    NamespaceDetails* details = _namespaceIndex.details(ns);
    if (details) {
        return;
    }
    _namespaceIndex.add_ns(txn, ns, DiskLoc(), false);
}

void MMAPV1DatabaseCatalogEntry::_init(OperationContext* txn) {
    WriteUnitOfWork wunit(txn);

    // Upgrade freelist
    const NamespaceString oldFreeList(name(), "$freelist");
    NamespaceDetails* freeListDetails = _namespaceIndex.details(oldFreeList.ns());
    if (freeListDetails) {
        if (!freeListDetails->firstExtent.isNull()) {
            _extentManager.freeExtents(
                txn, freeListDetails->firstExtent, freeListDetails->lastExtent);
        }

        _namespaceIndex.kill_ns(txn, oldFreeList.ns());
    }

    DataFileVersion version = _extentManager.getFileFormat(txn);
    if (version.isCompatibleWithCurrentCode() && !version.mayHave28Freelist()) {
        // Any DB that can be opened and written to gets this flag set.
        version.setMayHave28Freelist();
        _extentManager.setFileFormat(txn, version);
    }

    const NamespaceString nsi(name(), "system.indexes");
    const NamespaceString nsn(name(), "system.namespaces");

    bool isSystemNamespacesGoingToBeNew = _namespaceIndex.details(nsn.toString()) == NULL;
    bool isSystemIndexesGoingToBeNew = _namespaceIndex.details(nsi.toString()) == NULL;

    _ensureSystemCollection(txn, nsn.toString());
    _ensureSystemCollection(txn, nsi.toString());

    if (isSystemNamespacesGoingToBeNew) {
        txn->recoveryUnit()->registerChange(new EntryInsertion(nsn.toString(), this));
    }
    if (isSystemIndexesGoingToBeNew) {
        txn->recoveryUnit()->registerChange(new EntryInsertion(nsi.toString(), this));
    }

    Entry*& indexEntry = _collections[nsi.toString()];
    Entry*& nsEntry = _collections[nsn.toString()];

    NamespaceDetails* const indexDetails = _namespaceIndex.details(nsi.toString());
    NamespaceDetails* const nsDetails = _namespaceIndex.details(nsn.toString());

    // order has to be:
    // 1) ns rs
    // 2) i rs
    // 3) catalog entries

    if (!nsEntry) {
        nsEntry = new Entry();

        NamespaceDetailsRSV1MetaData* md =
            new NamespaceDetailsRSV1MetaData(nsn.toString(), nsDetails);
        nsEntry->recordStore.reset(
            new SimpleRecordStoreV1(txn, nsn.toString(), md, &_extentManager, false));
    }

    if (!indexEntry) {
        indexEntry = new Entry();

        NamespaceDetailsRSV1MetaData* md =
            new NamespaceDetailsRSV1MetaData(nsi.toString(), indexDetails);

        indexEntry->recordStore.reset(
            new SimpleRecordStoreV1(txn, nsi.toString(), md, &_extentManager, true));
    }

    if (isSystemIndexesGoingToBeNew) {
        _addNamespaceToNamespaceCollection(txn, nsi.toString(), NULL);
    }

    if (!nsEntry->catalogEntry) {
        nsEntry->catalogEntry.reset(
            new NamespaceDetailsCollectionCatalogEntry(nsn.toString(),
                                                       nsDetails,
                                                       nsEntry->recordStore.get(),
                                                       indexEntry->recordStore.get(),
                                                       this));
    }

    if (!indexEntry->catalogEntry) {
        indexEntry->catalogEntry.reset(
            new NamespaceDetailsCollectionCatalogEntry(nsi.toString(),
                                                       indexDetails,
                                                       nsEntry->recordStore.get(),
                                                       indexEntry->recordStore.get(),
                                                       this));
    }

    wunit.commit();

    // Now put everything in the cache of namespaces. None of the operations below do any
    // transactional operations.
    std::list<std::string> namespaces;
    _namespaceIndex.getCollectionNamespaces(&namespaces);

    for (std::list<std::string>::const_iterator i = namespaces.begin();
         i != namespaces.end();  // we add to the list in the loop so can't cache end().
         i++) {
        const std::string& ns = *i;
        Entry*& entry = _collections[ns];

        // The two cases where entry is not null is for system.indexes and system.namespaces,
        // which we manually instantiated above. It is OK to skip these two collections,
        // because they don't have indexes on them anyway.
        if (entry) {
            continue;
        }

        entry = new Entry();
        _insertInCache(txn, ns, entry);

        // Add the indexes on this namespace to the list of namespaces to load.
        std::vector<std::string> indexNames;
        entry->catalogEntry->getAllIndexes(txn, &indexNames);

        for (size_t i = 0; i < indexNames.size(); i++) {
            namespaces.push_back(IndexDescriptor::makeIndexNamespace(ns, indexNames[i]));
        }
    }
}

Status MMAPV1DatabaseCatalogEntry::createCollection(OperationContext* txn,
                                                    const StringData& ns,
                                                    const CollectionOptions& options,
                                                    bool allocateDefaultSpace) {
    if (_namespaceIndex.details(ns)) {
        return Status(ErrorCodes::NamespaceExists,
                      str::stream() << "namespace already exists: " << ns);
    }

    BSONObj optionsAsBSON = options.toBSON();
    _addNamespaceToNamespaceCollection(txn, ns, &optionsAsBSON);

    _namespaceIndex.add_ns(txn, ns, DiskLoc(), options.capped);
    NamespaceDetails* details = _namespaceIndex.details(ns);

    // Set the flags.
    NamespaceDetailsRSV1MetaData(ns, details).replaceUserFlags(txn, options.flags);

    if (options.capped && options.cappedMaxDocs > 0) {
        txn->recoveryUnit()->writingInt(details->maxDocsInCapped) = options.cappedMaxDocs;
    }

    Entry*& entry = _collections[ns.toString()];
    invariant(!entry);
    txn->recoveryUnit()->registerChange(new EntryInsertion(ns, this));
    entry = new Entry();
    _insertInCache(txn, ns, entry);

    if (allocateDefaultSpace) {
        RecordStoreV1Base* rs = _getRecordStore(ns);
        if (options.initialNumExtents > 0) {
            int size = _massageExtentSize(&_extentManager, options.cappedSize);
            for (int i = 0; i < options.initialNumExtents; i++) {
                rs->increaseStorageSize(txn, size, false);
            }
        } else if (!options.initialExtentSizes.empty()) {
            for (size_t i = 0; i < options.initialExtentSizes.size(); i++) {
                int size = options.initialExtentSizes[i];
                size = _massageExtentSize(&_extentManager, size);
                rs->increaseStorageSize(txn, size, false);
            }
        } else if (options.capped) {
            // normal
            do {
                // Must do this at least once, otherwise we leave the collection with no
                // extents, which is invalid.
                int sz =
                    _massageExtentSize(&_extentManager, options.cappedSize - rs->storageSize(txn));
                sz &= 0xffffff00;
                rs->increaseStorageSize(txn, sz, false);
            } while (rs->storageSize(txn) < options.cappedSize);
        } else {
            rs->increaseStorageSize(txn, _extentManager.initialSize(128), false);
        }
    }

    return Status::OK();
}

void MMAPV1DatabaseCatalogEntry::createNamespaceForIndex(OperationContext* txn,
                                                         const StringData& name) {
    // This is a simplified form of createCollection.
    invariant(!_namespaceIndex.details(name));

    _addNamespaceToNamespaceCollection(txn, name, NULL);
    _namespaceIndex.add_ns(txn, name, DiskLoc(), false);

    Entry*& entry = _collections[name.toString()];
    invariant(!entry);
    txn->recoveryUnit()->registerChange(new EntryInsertion(name, this));
    entry = new Entry();
    _insertInCache(txn, name, entry);
}

CollectionCatalogEntry* MMAPV1DatabaseCatalogEntry::getCollectionCatalogEntry(
    const StringData& ns) const {
    CollectionMap::const_iterator i = _collections.find(ns.toString());
    if (i == _collections.end()) {
        return NULL;
    }

    invariant(i->second->catalogEntry.get());
    return i->second->catalogEntry.get();
}

void MMAPV1DatabaseCatalogEntry::_insertInCache(OperationContext* txn,
                                                const StringData& ns,
                                                Entry* entry) {
    NamespaceDetails* details = _namespaceIndex.details(ns);
    invariant(details);

    entry->catalogEntry.reset(new NamespaceDetailsCollectionCatalogEntry(
        ns, details, _getNamespaceRecordStore(), _getIndexRecordStore(), this));

    auto_ptr<NamespaceDetailsRSV1MetaData> md(new NamespaceDetailsRSV1MetaData(ns, details));
    const NamespaceString nss(ns);

    if (details->isCapped) {
        entry->recordStore.reset(new CappedRecordStoreV1(
            txn, NULL, ns, md.release(), &_extentManager, nss.coll() == "system.indexes"));
    } else {
        entry->recordStore.reset(new SimpleRecordStoreV1(
            txn, ns, md.release(), &_extentManager, nss.coll() == "system.indexes"));
    }
}

RecordStore* MMAPV1DatabaseCatalogEntry::getRecordStore(const StringData& ns) const {
    return _getRecordStore(ns);
}

RecordStoreV1Base* MMAPV1DatabaseCatalogEntry::_getRecordStore(const StringData& ns) const {
    CollectionMap::const_iterator i = _collections.find(ns.toString());
    if (i == _collections.end()) {
        return NULL;
    }

    invariant(i->second->recordStore.get());
    return i->second->recordStore.get();
}

IndexAccessMethod* MMAPV1DatabaseCatalogEntry::getIndex(OperationContext* txn,
                                                        const CollectionCatalogEntry* collection,
                                                        IndexCatalogEntry* entry) {
    const string& type = entry->descriptor()->getAccessMethodName();

    string ns = collection->ns().ns();

    RecordStoreV1Base* rs = _getRecordStore(entry->descriptor()->indexNamespace());
    invariant(rs);

    std::auto_ptr<SortedDataInterface> btree(
        getMMAPV1Interface(entry->headManager(),
                           rs,
                           &rs->savedCursors,
                           entry->ordering(),
                           entry->descriptor()->indexNamespace(),
                           entry->descriptor()->version()));

    if (IndexNames::HASHED == type)
        return new HashAccessMethod(entry, btree.release());

    if (IndexNames::GEO_2DSPHERE == type)
        return new S2AccessMethod(entry, btree.release());

    if (IndexNames::TEXT == type)
        return new FTSAccessMethod(entry, btree.release());

    if (IndexNames::GEO_HAYSTACK == type)
        return new HaystackAccessMethod(entry, btree.release());

    if ("" == type)
        return new BtreeAccessMethod(entry, btree.release());

    if (IndexNames::GEO_2D == type)
        return new TwoDAccessMethod(entry, btree.release());

    log() << "Can't find index for keyPattern " << entry->descriptor()->keyPattern();
    fassertFailed(17489);
}

RecordStoreV1Base* MMAPV1DatabaseCatalogEntry::_getIndexRecordStore() {
    const NamespaceString nss(name(), "system.indexes");
    Entry* entry = _collections[nss.toString()];
    invariant(entry);

    return entry->recordStore.get();
}

RecordStoreV1Base* MMAPV1DatabaseCatalogEntry::_getNamespaceRecordStore() const {
    const NamespaceString nss(name(), "system.namespaces");
    CollectionMap::const_iterator i = _collections.find(nss.toString());
    invariant(i != _collections.end());

    return i->second->recordStore.get();
}

void MMAPV1DatabaseCatalogEntry::_addNamespaceToNamespaceCollection(OperationContext* txn,
                                                                    const StringData& ns,
                                                                    const BSONObj* options) {
    if (nsToCollectionSubstring(ns) == "system.namespaces") {
        // system.namespaces holds all the others, so it is not explicitly listed in the catalog.
        return;
    }

    BSONObjBuilder b;
    b.append("name", ns);
    if (options && !options->isEmpty()) {
        b.append("options", *options);
    }

    const BSONObj obj = b.done();

    RecordStoreV1Base* rs = _getNamespaceRecordStore();
    invariant(rs);

    StatusWith<RecordId> loc = rs->insertRecord(txn, obj.objdata(), obj.objsize(), false);
    massertStatusOK(loc.getStatus());
}

void MMAPV1DatabaseCatalogEntry::_removeNamespaceFromNamespaceCollection(OperationContext* txn,
                                                                         const StringData& ns) {
    if (nsToCollectionSubstring(ns) == "system.namespaces") {
        // system.namespaces holds all the others, so it is not explicitly listed in the catalog.
        return;
    }

    RecordStoreV1Base* rs = _getNamespaceRecordStore();
    invariant(rs);

    scoped_ptr<RecordIterator> it(rs->getIterator(txn));
    while (!it->isEOF()) {
        RecordId loc = it->getNext();
        BSONObj entry = it->dataFor(loc).toBson();
        BSONElement name = entry["name"];
        if (name.type() == String && name.String() == ns) {
            rs->deleteRecord(txn, loc);
            break;
        }
    }
}

CollectionOptions MMAPV1DatabaseCatalogEntry::getCollectionOptions(OperationContext* txn,
                                                                   const StringData& ns) const {
    if (nsToCollectionSubstring(ns) == "system.namespaces") {
        return CollectionOptions();
    }

    RecordStoreV1Base* rs = _getNamespaceRecordStore();
    invariant(rs);

    scoped_ptr<RecordIterator> it(rs->getIterator(txn));
    while (!it->isEOF()) {
        RecordId loc = it->getNext();
        BSONObj entry = it->dataFor(loc).toBson();
        BSONElement name = entry["name"];
        if (name.type() == String && name.String() == ns) {
            CollectionOptions options;
            if (entry["options"].isABSONObj()) {
                Status status = options.parse(entry["options"].Obj());
                fassert(18523, status);
            }
            return options;
        }
    }

    return CollectionOptions();
}
}  // namespace mongo
