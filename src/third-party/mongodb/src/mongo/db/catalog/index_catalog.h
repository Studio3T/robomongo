// index_catalog.h

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

#pragma once

#include <vector>

#include "mongo/db/catalog/index_catalog_entry.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/record_id.h"
#include "mongo/platform/unordered_map.h"

namespace mongo {

class Client;
class Collection;

class IndexDescriptor;
class IndexAccessMethod;

/**
 * how many: 1 per Collection
 * lifecycle: attached to a Collection
 */
class IndexCatalog {
public:
    IndexCatalog(Collection* collection);
    ~IndexCatalog();

    // must be called before used
    Status init(OperationContext* txn);

    bool ok() const;

    // ---- accessors -----

    bool haveAnyIndexes() const;
    int numIndexesTotal(OperationContext* txn) const;
    int numIndexesReady(OperationContext* txn) const;
    int numIndexesInProgress(OperationContext* txn) const {
        return numIndexesTotal(txn) - numIndexesReady(txn);
    }

    /**
     * this is in "alive" until the Collection goes away
     * in which case everything from this tree has to go away
     */

    bool haveIdIndex(OperationContext* txn) const;

    /**
     * Returns the spec for the id index to create by default for this collection.
     */
    BSONObj getDefaultIdIndexSpec() const;

    IndexDescriptor* findIdIndex(OperationContext* txn) const;

    /**
     * @return null if cannot find
     */
    IndexDescriptor* findIndexByName(OperationContext* txn,
                                     const StringData& name,
                                     bool includeUnfinishedIndexes = false) const;

    /**
     * @return null if cannot find
     */
    IndexDescriptor* findIndexByKeyPattern(OperationContext* txn,
                                           const BSONObj& key,
                                           bool includeUnfinishedIndexes = false) const;

    /* Returns the index entry for the first index whose prefix contains
     * 'keyPattern'. If 'requireSingleKey' is true, skip indices that contain
     * array attributes. Otherwise, returns NULL.
     */
    IndexDescriptor* findIndexByPrefix(OperationContext* txn,
                                       const BSONObj& keyPattern,
                                       bool requireSingleKey) const;

    void findIndexByType(OperationContext* txn,
                         const std::string& type,
                         std::vector<IndexDescriptor*>& matches,
                         bool includeUnfinishedIndexes = false) const;


    /**
     * Reload the index definition for 'oldDesc' from the CollectionCatalogEntry.  'oldDesc'
     * must be a ready index that is already registered with the index catalog.  Returns an
     * unowned pointer to the descriptor for the new index definition.
     *
     * Use this method to notify the IndexCatalog that the spec for this index has changed.
     *
     * It is invalid to dereference 'oldDesc' after calling this method.  This method broadcasts
     * an invalidateAll() on the cursor manager to notify other users of the IndexCatalog that
     * this descriptor is now invalid.
     */
    const IndexDescriptor* refreshEntry(OperationContext* txn, const IndexDescriptor* oldDesc);

    // never returns NULL
    const IndexCatalogEntry* getEntry(const IndexDescriptor* desc) const;

    IndexAccessMethod* getIndex(const IndexDescriptor* desc);
    const IndexAccessMethod* getIndex(const IndexDescriptor* desc) const;

    /**
     * Returns a not-ok Status if there are any unfinished index builds. No new indexes should
     * be built when in this state.
     */
    Status checkUnfinished() const;

    class IndexIterator {
    public:
        bool more();
        IndexDescriptor* next();

        // returns the access method for the last return IndexDescriptor
        IndexAccessMethod* accessMethod(IndexDescriptor* desc);

    private:
        IndexIterator(OperationContext* txn,
                      const IndexCatalog* cat,
                      bool includeUnfinishedIndexes);

        void _advance();

        bool _includeUnfinishedIndexes;

        OperationContext* const _txn;
        const IndexCatalog* _catalog;
        IndexCatalogEntryContainer::const_iterator _iterator;

        bool _start;  // only true before we've called next() or more()

        IndexCatalogEntry* _prev;
        IndexCatalogEntry* _next;

        friend class IndexCatalog;
    };

    IndexIterator getIndexIterator(OperationContext* txn, bool includeUnfinishedIndexes) const {
        return IndexIterator(txn, this, includeUnfinishedIndexes);
    };

    // ---- index set modifiers ------

    /**
     * Call this only on an empty collection from inside a WriteUnitOfWork. Index creation on an
     * empty collection can be rolled back as part of a larger WUOW.
     */
    Status createIndexOnEmptyCollection(OperationContext* txn, BSONObj spec);

    StatusWith<BSONObj> prepareSpecForCreate(OperationContext* txn, const BSONObj& original) const;

    Status dropAllIndexes(OperationContext* txn, bool includingIdIndex);

    Status dropIndex(OperationContext* txn, IndexDescriptor* desc);

    /**
     * will drop all incompleted indexes and return specs
     * after this, the indexes can be rebuilt
     */
    std::vector<BSONObj> getAndClearUnfinishedIndexes(OperationContext* txn);


    struct IndexKillCriteria {
        std::string ns;
        std::string name;
        BSONObj key;
    };

    /**
     * Registers an index build in an internal tracking map, for use with
     * killMatchingIndexBuilds().  The opNum and descriptor provided must remain active
     * for as long as the entry exists in the map.  The opNum provided must correspond to
     * an operation building only one index, in the background.
     * This function is intended for replication to use for tracking and managing background
     * index builds.  It is expected that the caller has already taken steps to serialize
     * calls to this function.
     */
    void registerIndexBuild(IndexDescriptor* descriptor, unsigned int opNum);

    /**
     * Removes an index build from the map, upon completion or termination of the index build.
     * This function is intended for replication to use for tracking and managing background
     * index builds.  It is expected that the caller has already taken steps to serialize
     * calls to this function.
     */
    void unregisterIndexBuild(IndexDescriptor* descriptor);

    /**
     * Given some criteria, searches through all in-progress index builds
     * and kills ones that match. (namespace, index name, and/or index key spec)
     * Returns the list of index specs that were killed, for use in restarting them later.
     */
    std::vector<BSONObj> killMatchingIndexBuilds(const IndexKillCriteria& criteria);

    // ---- modify single index

    bool isMultikey(OperationContext* txn, const IndexDescriptor* idex);

    // --- these probably become private?


    /**
     * disk creation order
     * 1) system.indexes entry
     * 2) collection's NamespaceDetails
     *    a) info + head
     *    b) _indexBuildsInProgress++
     * 3) indexes entry in .ns file
     * 4) system.namespaces entry for index ns
     */
    class IndexBuildBlock {
        MONGO_DISALLOW_COPYING(IndexBuildBlock);

    public:
        IndexBuildBlock(OperationContext* txn, Collection* collection, const BSONObj& spec);

        ~IndexBuildBlock();

        Status init();

        void success();

        /**
         * index build failed, clean up meta data
         */
        void fail();

        IndexCatalogEntry* getEntry() {
            return _entry;
        }

    private:
        Collection* const _collection;
        IndexCatalog* const _catalog;
        const std::string _ns;

        BSONObj _spec;

        std::string _indexName;
        std::string _indexNamespace;

        IndexCatalogEntry* _entry;
        bool _inProgress;

        OperationContext* _txn;
    };

    // ----- data modifiers ------

    // this throws for now
    Status indexRecord(OperationContext* txn, const BSONObj& obj, const RecordId& loc);

    void unindexRecord(OperationContext* txn, const BSONObj& obj, const RecordId& loc, bool noWarn);

    // ------- temp internal -------

    std::string getAccessMethodName(OperationContext* txn, const BSONObj& keyPattern) {
        return _getAccessMethodName(txn, keyPattern);
    }

    Status _upgradeDatabaseMinorVersionIfNeeded(OperationContext* txn,
                                                const std::string& newPluginName);

    // public static helpers

    static BSONObj fixIndexKey(const BSONObj& key);

private:
    typedef unordered_map<IndexDescriptor*, unsigned int> InProgressIndexesMap;

    static const BSONObj _idObj;  // { _id : 1 }

    bool _shouldOverridePlugin(OperationContext* txn, const BSONObj& keyPattern) const;

    /**
     * This differs from IndexNames::findPluginName in that returns the plugin name we *should*
     * use, not the plugin name inside of the provided key pattern.  To understand when these
     * differ, see shouldOverridePlugin.
     */
    std::string _getAccessMethodName(OperationContext* txn, const BSONObj& keyPattern) const;

    void _checkMagic() const;

    Status _indexRecord(OperationContext* txn,
                        IndexCatalogEntry* index,
                        const BSONObj& obj,
                        const RecordId& loc);

    Status _unindexRecord(OperationContext* txn,
                          IndexCatalogEntry* index,
                          const BSONObj& obj,
                          const RecordId& loc,
                          bool logIfError);

    /**
     * this does no sanity checks
     */
    Status _dropIndex(OperationContext* txn, IndexCatalogEntry* entry);

    // just does disk hanges
    // doesn't change memory state, etc...
    void _deleteIndexFromDisk(OperationContext* txn,
                              const std::string& indexName,
                              const std::string& indexNamespace);

    // descriptor ownership passes to _setupInMemoryStructures
    // initFromDisk: Avoids registering a change to undo this operation when set to true.
    //               You must set this flag if calling this function outside of a UnitOfWork.
    IndexCatalogEntry* _setupInMemoryStructures(OperationContext* txn,
                                                IndexDescriptor* descriptor,
                                                bool initFromDisk);

    // Apply a set of transformations to the user-provided index object 'spec' to make it
    // conform to the standard for insertion.  This function adds the 'v' field if it didn't
    // exist, removes the '_id' field if it exists, applies plugin-level transformations if
    // appropriate, etc.
    static BSONObj _fixIndexSpec(const BSONObj& spec);

    Status _isSpecOk(const BSONObj& spec) const;

    Status _doesSpecConflictWithExisting(OperationContext* txn, const BSONObj& spec) const;

    int _magic;
    Collection* const _collection;

    IndexCatalogEntryContainer _entries;

    // These are the index specs of indexes that were "leftover".
    // "Leftover" means they were unfinished when a mongod shut down.
    // Certain operations are prohibited until someone fixes.
    // Retrieve by calling getAndClearUnfinishedIndexes().
    std::vector<BSONObj> _unfinishedIndexes;

    // Track in-progress index builds, in order to find and stop them when necessary.
    InProgressIndexesMap _inProgressIndexes;
};
}
