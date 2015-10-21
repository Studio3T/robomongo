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
#include "mongo/util/mongoutils/str.h"

namespace mongo {

namespace str = mongoutils::str;

struct UpdateResult {
    UpdateResult(bool existing_,
                 bool modifiers_,
                 unsigned long long numDocsModified_,
                 unsigned long long numMatched_,
                 const BSONObj& upsertedObject_,
                 const BSONObj& newObj_);


    // if existing objects were modified
    const bool existing;

    // was this a $ mod
    const bool modifiers;

    // how many docs updated
    const long long numDocsModified;

    // how many docs seen by update
    const long long numMatched;

    // if something was upserted, the new _id of the object
    BSONObj upserted;

    // For a non-multi update, the new version of the document. If we did an insert, this
    // is the full document that got inserted (whereas 'upserted' is just the _id field).
    BSONObj newObj;

    const std::string toString() const {
        return str::stream() << " upserted: " << upserted << " modifiers: " << modifiers
                             << " existing: " << existing << " numDocsModified: " << numDocsModified
                             << " numMatched: " << numMatched << " newObj: " << newObj;
    }
};

}  // namespace mongo
