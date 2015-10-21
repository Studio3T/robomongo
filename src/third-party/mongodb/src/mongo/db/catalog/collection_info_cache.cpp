// collection_info_cache.cpp

/**
*    Copyright (C) 2013 10gen Inc.
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

#include "mongo/db/catalog/collection_info_cache.h"

#include "mongo/db/catalog/collection.h"
#include "mongo/db/concurrency/d_concurrency.h"
#include "mongo/db/fts/fts_spec.h"
#include "mongo/db/index/index_descriptor.h"
#include "mongo/db/index_legacy.h"
#include "mongo/db/query/plan_cache.h"
#include "mongo/util/debug_util.h"
#include "mongo/util/log.h"


namespace mongo {

CollectionInfoCache::CollectionInfoCache(Collection* collection)
    : _collection(collection),
      _keysComputed(false),
      _planCache(new PlanCache(collection->ns().ns())),
      _querySettings(new QuerySettings()) {}

void CollectionInfoCache::reset(OperationContext* txn) {
    LOG(1) << _collection->ns().ns() << ": clearing plan cache - collection info cache reset";
    clearQueryCache();
    _keysComputed = false;
    computeIndexKeys(txn);
    // query settings is not affected by info cache reset.
    // index filters should persist throughout life of collection
}

const UpdateIndexData& CollectionInfoCache::indexKeys(OperationContext* txn) const {
    // This requires "some" lock, and MODE_IS is an expression for that, for now.
    dassert(txn->lockState()->isCollectionLockedForMode(_collection->ns().ns(), MODE_IS));
    invariant(_keysComputed);
    return _indexedPaths;
}

void CollectionInfoCache::computeIndexKeys(OperationContext* txn) {
    // This function modified objects attached to the Collection so we need a write lock
    invariant(txn->lockState()->isCollectionLockedForMode(_collection->ns().ns(), MODE_X));
    _indexedPaths.clear();

    IndexCatalog::IndexIterator i = _collection->getIndexCatalog()->getIndexIterator(txn, true);
    while (i.more()) {
        IndexDescriptor* descriptor = i.next();

        if (descriptor->getAccessMethodName() != IndexNames::TEXT) {
            BSONObj key = descriptor->keyPattern();
            BSONObjIterator j(key);
            while (j.more()) {
                BSONElement e = j.next();
                _indexedPaths.addPath(e.fieldName());
            }
        } else {
            fts::FTSSpec ftsSpec(descriptor->infoObj());

            if (ftsSpec.wildcard()) {
                _indexedPaths.allPathsIndexed();
            } else {
                for (size_t i = 0; i < ftsSpec.numExtraBefore(); ++i) {
                    _indexedPaths.addPath(ftsSpec.extraBefore(i));
                }
                for (fts::Weights::const_iterator it = ftsSpec.weights().begin();
                     it != ftsSpec.weights().end();
                     ++it) {
                    _indexedPaths.addPath(it->first);
                }
                for (size_t i = 0; i < ftsSpec.numExtraAfter(); ++i) {
                    _indexedPaths.addPath(ftsSpec.extraAfter(i));
                }
                // Any update to a path containing "language" as a component could change the
                // language of a subdocument.  Add the override field as a path component.
                _indexedPaths.addPathComponent(ftsSpec.languageOverrideField());
            }
        }
    }

    _keysComputed = true;
}

void CollectionInfoCache::notifyOfWriteOp() {
    if (NULL != _planCache.get()) {
        _planCache->notifyOfWriteOp();
    }
}

void CollectionInfoCache::clearQueryCache() {
    if (NULL != _planCache.get()) {
        _planCache->clear();
    }
}

PlanCache* CollectionInfoCache::getPlanCache() const {
    return _planCache.get();
}

QuerySettings* CollectionInfoCache::getQuerySettings() const {
    return _querySettings.get();
}
}
