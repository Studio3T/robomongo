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

#include "mongo/db/jsobj.h"
#include "mongo/db/matcher/expression_parser.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

class ParsedProjection {
public:
    // TODO: this is duplicated in here and in the proj exec code.  When we have
    // ProjectionExpression we can remove dups.
    enum ArrayOpType { ARRAY_OP_NORMAL = 0, ARRAY_OP_ELEM_MATCH, ARRAY_OP_POSITIONAL };

    /**
     * Parses the projection 'spec' and checks its validity with respect to the query 'query'.
     * Puts covering information into 'out'.
     *
     * Returns Status::OK() if it's a valid spec.
     * Returns a Status indicating how it's invalid otherwise.
     */
    static Status make(const BSONObj& spec,
                       const MatchExpression* const query,
                       ParsedProjection** out,
                       const MatchExpressionParser::WhereCallback& whereCallback =
                           MatchExpressionParser::WhereCallback());

    /**
     * Is the full document required to compute this projection?
     */
    bool requiresDocument() const {
        return _requiresDocument;
    }

    /**
     * If requiresDocument() == false, what fields are required to compute
     * the projection?
     */
    const std::vector<std::string>& getRequiredFields() const {
        return _requiredFields;
    }

    /**
     * Get the raw BSONObj proj spec obj
     */
    const BSONObj& getProjObj() const {
        return _source;
    }

    /**
     * Does the projection want geoNear metadata?  If so any geoNear stage should include them.
     */
    bool wantGeoNearDistance() const {
        return _wantGeoNearDistance;
    }

    bool wantGeoNearPoint() const {
        return _wantGeoNearPoint;
    }

    bool wantIndexKey() const {
        return _returnKey;
    }

private:
    /**
     * Must go through ::make
     */
    ParsedProjection() : _requiresDocument(true) {}

    /**
     * Returns true if field name refers to a positional projection.
     */
    static bool _isPositionalOperator(const char* fieldName);

    /**
     * Returns true if the MatchExpression 'query' queries against
     * the field named by 'matchfield'. This deeply traverses logical
     * nodes in the matchfield and returns true if any of the children
     * have the field (so if 'query' is {$and: [{a: 1}, {b: 1}]} and
     * 'matchfield' is "b", the return value is true).
     *
     * Does not take ownership of 'query'.
     */
    static bool _hasPositionalOperatorMatch(const MatchExpression* const query,
                                            const std::string& matchfield);

    // TODO: stringdata?
    std::vector<std::string> _requiredFields;

    bool _requiresDocument;

    BSONObj _source;

    bool _wantGeoNearDistance;

    bool _wantGeoNearPoint;

    bool _returnKey;
};

}  // namespace mongo
