// kv_catalog.h

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

#pragma once

#include <map>
#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "mongo/base/string_data.h"
#include "mongo/db/catalog/collection_options.h"
#include "mongo/db/record_id.h"
#include "mongo/db/storage/bson_collection_catalog_entry.h"

namespace mongo {

class OperationContext;
class RecordStore;

class KVCatalog {
public:
    /**
     * @param rs - does NOT take ownership
     */
    KVCatalog(RecordStore* rs, bool isRsThreadSafe, bool directoryPerDb, bool directoryForIndexes);
    ~KVCatalog();

    void init(OperationContext* opCtx);

    void getAllCollections(std::vector<std::string>* out) const;

    /**
     * @return error or ident for instance
     */
    Status newCollection(OperationContext* opCtx,
                         const StringData& ns,
                         const CollectionOptions& options);

    std::string getCollectionIdent(const StringData& ns) const;

    std::string getIndexIdent(OperationContext* opCtx,
                              const StringData& ns,
                              const StringData& idName) const;

    const BSONCollectionCatalogEntry::MetaData getMetaData(OperationContext* opCtx,
                                                           const StringData& ns);
    void putMetaData(OperationContext* opCtx,
                     const StringData& ns,
                     BSONCollectionCatalogEntry::MetaData& md);

    Status renameCollection(OperationContext* opCtx,
                            const StringData& fromNS,
                            const StringData& toNS,
                            bool stayTemp);

    Status dropCollection(OperationContext* opCtx, const StringData& ns);

    std::vector<std::string> getAllIdentsForDB(const StringData& db) const;
    std::vector<std::string> getAllIdents(OperationContext* opCtx) const;

    bool isUserDataIdent(const StringData& ident) const;

private:
    class AddIdentChange;
    class RemoveIdentChange;

    BSONObj _findEntry(OperationContext* opCtx, const StringData& ns, RecordId* out = NULL) const;

    /**
     * Generates a new unique identifier for a new "thing".
     * @param ns - the containing ns
     * @param kind - what this "thing" is, likely collection or index
     */
    std::string _newUniqueIdent(const StringData& ns, const char* kind);

    // Helpers only used by constructor and init(). Don't call from elsewhere.
    static std::string _newRand();
    bool _hasEntryCollidingWithRand() const;

    RecordStore* _rs;  // not owned
    const bool _isRsThreadSafe;
    const bool _directoryPerDb;
    const bool _directoryForIndexes;

    // These two are only used for ident generation inside _newUniqueIdent.
    std::string _rand;  // effectively const after init() returns
    AtomicUInt64 _next;

    struct Entry {
        Entry() {}
        Entry(std::string i, RecordId l) : ident(i), storedLoc(l) {}
        std::string ident;
        RecordId storedLoc;
    };
    typedef std::map<std::string, Entry> NSToIdentMap;
    NSToIdentMap _idents;
    mutable boost::mutex _identsLock;
};
}
