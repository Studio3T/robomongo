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

#include <boost/scoped_ptr.hpp>

#include "mongo/base/disallow_copying.h"
#include "mongo/bson/mutable/document.h"
#include "mongo/db/field_ref.h"
#include "mongo/db/ops/modifier_interface.h"

namespace mongo {

class LogBuilder;

class ModifierAddToSet : public ModifierInterface {
    MONGO_DISALLOW_COPYING(ModifierAddToSet);

public:
    ModifierAddToSet();
    virtual ~ModifierAddToSet();

    /** Goes over the array item(s) that are going to be set- unioned and converts them
     *  internally to a mutable bson. Both single and $each forms are supported. Returns OK
     *  if the item(s) are valid otherwise returns a status describing the error.
     */
    virtual Status init(const BSONElement& modExpr, const Options& opts, bool* positional = NULL);

    /** Decides which portion of the array items that are going to be set-unioned to root's
     *  document and fills in 'execInfo' accordingly. Returns OK if the document has a
     *  valid array to set-union to, othwise returns a status describing the error.
     */
    virtual Status prepare(mutablebson::Element root,
                           const StringData& matchedField,
                           ExecInfo* execInfo);

    /** Updates the Element used in prepare with the effects of the $addToSet operation. */
    virtual Status apply() const;

    /** Converts the effects of this $addToSet into one or more equivalent $set operations. */
    virtual Status log(LogBuilder* logBuilder) const;

private:
    // Access to each component of fieldName that's the target of this mod.
    FieldRef _fieldRef;

    // 0 or index for $-positional in _fieldRef.
    size_t _posDollar;

    // Array of values to be set-union'ed onto target.
    mutablebson::Document _valDoc;
    mutablebson::Element _val;

    struct PreparedState;
    boost::scoped_ptr<PreparedState> _preparedState;
};

}  // namespace mongo
