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

#include "mongo/base/disallow_copying.h"
#include "mongo/base/status.h"
#include "mongo/db/ops/update_driver.h"

namespace mongo {

class CanonicalQuery;
class OperationContext;
class UpdateRequest;

/**
 * This class takes a pointer to an UpdateRequest, and converts that request into a parsed form
 * via the parseRequest() method. A ParsedUpdate can then be used to retrieve a PlanExecutor
 * capable of executing the update.
 *
 * No locks need to be held during parsing.
 *
 * The query part of the update is parsed to a CanonicalQuery, and the update part is parsed
 * using the UpdateDriver.
 */
class ParsedUpdate {
    MONGO_DISALLOW_COPYING(ParsedUpdate);

public:
    /**
     * Constructs a parsed update.
     *
     * The object pointed to by "request" must stay in scope for the life of the constructed
     * ParsedUpdate.
     */
    ParsedUpdate(OperationContext* txn, const UpdateRequest* request);

    /**
     * Parses the update request to a canonical query and an update driver. On success, the
     * parsed update can be used to create a PlanExecutor for this update.
     */
    Status parseRequest();

    /**
     * As an optimization, we do not create a canonical query if the predicate is a simple
     * _id equality. This method can be used to force full parsing to a canonical query,
     * as a fallback if the idhack path is not available (e.g. no _id index).
     */
    Status parseQueryToCQ();

    /**
     * Get the raw request.
     */
    const UpdateRequest* getRequest() const;

    /**
     * Get a pointer to the update driver, the abstraction which both parses the update and
     * is capable of applying mods / computing damages.
     */
    UpdateDriver* getDriver();

    /**
     * Is this update allowed to yield?
     */
    bool canYield() const;

    /**
     * Is this update supposed to be isolated?
     */
    bool isIsolated() const;

    /**
     * As an optimization, we don't create a canonical query for updates with simple _id
     * queries. Use this method to determine whether or not we actually parsed the query.
     */
    bool hasParsedQuery() const;

    /**
     * Releases ownership of the canonical query to the caller.
     */
    CanonicalQuery* releaseParsedQuery();

private:
    /**
     * Parses the query portion of the update request.
     */
    Status parseQuery();

    /**
     * Parses the update-descriptor portion of the update request.
     */
    Status parseUpdate();

    // Unowned pointer to the transactional context.
    OperationContext* _txn;

    // Unowned pointer to the request object to process.
    const UpdateRequest* const _request;

    // Driver for processing updates on matched documents.
    UpdateDriver _driver;

    // Parsed query object, or NULL if the query proves to be an id hack query.
    std::auto_ptr<CanonicalQuery> _canonicalQuery;
};

}  // namespace mongo
