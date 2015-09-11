// in_memory_engine.h

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

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "mongo/db/storage/kv/kv_engine.h"
#include "mongo/util/string_map.h"

namespace mongo {

class InMemoryEngine : public KVEngine {
public:
    virtual RecoveryUnit* newRecoveryUnit();

    virtual Status createRecordStore(OperationContext* opCtx,
                                     const StringData& ns,
                                     const StringData& ident,
                                     const CollectionOptions& options);

    virtual RecordStore* getRecordStore(OperationContext* opCtx,
                                        const StringData& ns,
                                        const StringData& ident,
                                        const CollectionOptions& options);

    virtual Status createSortedDataInterface(OperationContext* opCtx,
                                             const StringData& ident,
                                             const IndexDescriptor* desc);

    virtual SortedDataInterface* getSortedDataInterface(OperationContext* opCtx,
                                                        const StringData& ident,
                                                        const IndexDescriptor* desc);

    virtual Status dropIdent(OperationContext* opCtx, const StringData& ident);

    virtual bool supportsDocLocking() const {
        return false;
    }

    virtual bool supportsDirectoryPerDB() const {
        return false;
    }

    /**
     * This is sort of strange since "durable" has no meaning...
     */
    virtual bool isDurable() const {
        return true;
    }

    virtual int64_t getIdentSize(OperationContext* opCtx, const StringData& ident);

    virtual Status repairIdent(OperationContext* opCtx, const StringData& ident) {
        return Status::OK();
    }

    virtual void cleanShutdown(){};

    virtual bool hasIdent(OperationContext* opCtx, const StringData& ident) const {
        return _dataMap.find(ident) != _dataMap.end();
        ;
    }

    std::vector<std::string> getAllIdents(OperationContext* opCtx) const;

private:
    typedef StringMap<boost::shared_ptr<void>> DataMap;

    mutable boost::mutex _mutex;
    DataMap _dataMap;  // All actual data is owned in here
};
}
