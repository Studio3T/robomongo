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

#pragma once

#include "mongo/db/jsobj.h"
#include "mongo/db/curop.h"
#include "mongo/db/namespace_string.h"
#include "mongo/db/query/explain.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

namespace str = mongoutils::str;

class FieldRef;
class UpdateLifecycle;

class UpdateRequest {
public:
    inline UpdateRequest(const NamespaceString& nsString)
        : _nsString(nsString),
          _god(false),
          _upsert(false),
          _multi(false),
          _callLogOp(false),
          _fromMigration(false),
          _fromReplication(false),
          _lifecycle(NULL),
          _isExplain(false),
          _storeResultDoc(false),
          _yieldPolicy(PlanExecutor::YIELD_MANUAL) {}

    const NamespaceString& getNamespaceString() const {
        return _nsString;
    }

    inline void setQuery(const BSONObj& query) {
        _query = query;
    }

    inline const BSONObj& getQuery() const {
        return _query;
    }

    inline void setUpdates(const BSONObj& updates) {
        _updates = updates;
    }

    inline const BSONObj& getUpdates() const {
        return _updates;
    }

    // Please see documentation on the private members matching these names for
    // explanations of the following fields.

    inline void setGod(bool value = true) {
        _god = value;
    }

    bool isGod() const {
        return _god;
    }

    inline void setUpsert(bool value = true) {
        _upsert = value;
    }

    bool isUpsert() const {
        return _upsert;
    }

    inline void setMulti(bool value = true) {
        _multi = value;
    }

    bool isMulti() const {
        return _multi;
    }

    inline void setUpdateOpLog(bool value = true) {
        _callLogOp = value;
    }

    bool shouldCallLogOp() const {
        return _callLogOp;
    }

    inline void setFromMigration(bool value = true) {
        _fromMigration = value;
    }

    bool isFromMigration() const {
        return _fromMigration;
    }

    inline void setFromReplication(bool value = true) {
        _fromReplication = value;
    }

    bool isFromReplication() const {
        return _fromReplication;
    }

    inline void setLifecycle(UpdateLifecycle* value) {
        _lifecycle = value;
    }

    inline UpdateLifecycle* getLifecycle() const {
        return _lifecycle;
    }

    inline void setExplain(bool value = true) {
        _isExplain = value;
    }

    inline bool isExplain() const {
        return _isExplain;
    }

    inline void setStoreResultDoc(bool value = true) {
        _storeResultDoc = value;
    }

    inline bool shouldStoreResultDoc() const {
        return _storeResultDoc;
    }

    inline void setYieldPolicy(PlanExecutor::YieldPolicy yieldPolicy) {
        _yieldPolicy = yieldPolicy;
    }

    inline PlanExecutor::YieldPolicy getYieldPolicy() const {
        return _yieldPolicy;
    }

    const std::string toString() const {
        return str::stream() << " query: " << _query << " updated: " << _updates << " god: " << _god
                             << " upsert: " << _upsert << " multi: " << _multi
                             << " callLogOp: " << _callLogOp << " fromMigration: " << _fromMigration
                             << " fromReplications: " << _fromReplication
                             << " isExplain: " << _isExplain;
    }

private:
    const NamespaceString& _nsString;

    // Contains the query that selects documents to update.
    BSONObj _query;

    // Contains the modifiers to apply to matched objects, or a replacement document.
    BSONObj _updates;

    // Flags controlling the update.

    // God bypasses _id checking and index generation. It is only used on behalf of system
    // updates, never user updates.
    bool _god;

    // True if this should insert if no matching document is found.
    bool _upsert;

    // True if this update is allowed to affect more than one document.
    bool _multi;

    // True if the effects of the update should be written to the oplog.
    bool _callLogOp;

    // True if this update is on behalf of a chunk migration.
    bool _fromMigration;

    // True if this update is being applied during the application for the oplog.
    bool _fromReplication;

    // The lifecycle data, and events used during the update request.
    UpdateLifecycle* _lifecycle;

    // Whether or not we are requesting an explained update. Explained updates are read-only.
    bool _isExplain;

    // Whether or not we keep an owned copy of the resulting document for a non-multi update.
    // This allows someone executing an update to retrieve the resulting document without
    // another query once the update is complete.
    //
    // It is illegal to use this flag in combination with the '_multi' flag, and doing so will
    // trigger an invariant check.
    bool _storeResultDoc;

    // Whether or not the update should yield. Defaults to YIELD_MANUAL.
    PlanExecutor::YieldPolicy _yieldPolicy;
};

}  // namespace mongo
