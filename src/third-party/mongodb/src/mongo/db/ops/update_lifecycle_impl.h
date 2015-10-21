/**
 *    Copyright (C) 2013 MongoDB Inc.
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

#include "mongo/base/disallow_copying.h"
#include "mongo/db/namespace_string.h"
#include "mongo/db/ops/update_lifecycle.h"
#include "mongo/db/catalog/collection.h"

namespace mongo {

class UpdateLifecycleImpl : public UpdateLifecycle {
    MONGO_DISALLOW_COPYING(UpdateLifecycleImpl);

public:
    /**
     * ignoreVersion is for shard version checking and
     * means that version checks will not be done
     *
     * nsString represents the namespace for the
     */
    UpdateLifecycleImpl(bool ignoreVersion, const NamespaceString& nsString);

    virtual void setCollection(Collection* collection);

    virtual bool canContinue() const;

    virtual const UpdateIndexData* getIndexKeys(OperationContext* opCtx) const;

    virtual const std::vector<FieldRef*>* getImmutableFields() const;

private:
    Collection* _collection;
    const NamespaceString& _nsString;
    ChunkVersion _shardVersion;
};

} /* namespace mongo */
