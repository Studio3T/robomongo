/**
*    Copyright (C) 2011 10gen Inc.
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
*/

#include "pch.h"

#include "db/pipeline/document_source.h"

#include "db/jsobj.h"
#include "db/pipeline/document.h"
#include "db/pipeline/expression.h"
#include "db/pipeline/expression_context.h"
#include "db/pipeline/value.h"

namespace mongo {
    const char DocumentSourceLimit::limitName[] = "$limit";

    DocumentSourceLimit::DocumentSourceLimit(const intrusive_ptr<ExpressionContext> &pExpCtx,
                                             long long limit)
        : SplittableDocumentSource(pExpCtx)
        , limit(limit)
        , count(0)
    {}

    DocumentSourceLimit::~DocumentSourceLimit() {
    }

    const char *DocumentSourceLimit::getSourceName() const {
        return limitName;
    }

    bool DocumentSourceLimit::coalesce(
        const intrusive_ptr<DocumentSource> &pNextSource) {
        DocumentSourceLimit *pLimit =
            dynamic_cast<DocumentSourceLimit *>(pNextSource.get());

        /* if it's not another $skip, we can't coalesce */
        if (!pLimit)
            return false;

        /* we need to limit by the minimum of the two limits */
        if (pLimit->limit < limit)
            limit = pLimit->limit;
        return true;
    }

    bool DocumentSourceLimit::eof() {
        return pSource->eof() || count >= limit;
    }

    bool DocumentSourceLimit::advance() {
        DocumentSource::advance(); // check for interrupts

        ++count;
        if (count >= limit) {
            // This is required for the DocumentSourceCursor to release its read lock, see
            // SERVER-6123.
            pSource->dispose();

            return false;
        }
        return pSource->advance();
    }

    Document DocumentSourceLimit::getCurrent() {
        return pSource->getCurrent();
    }

    void DocumentSourceLimit::sourceToBson(
        BSONObjBuilder *pBuilder, bool explain) const {
        pBuilder->append("$limit", limit);
    }

    intrusive_ptr<DocumentSourceLimit> DocumentSourceLimit::create(
            const intrusive_ptr<ExpressionContext> &pExpCtx,
            long long limit) {
        uassert(15958, "the limit must be positive",
                limit > 0);
        return new DocumentSourceLimit(pExpCtx, limit);
    }

    intrusive_ptr<DocumentSource> DocumentSourceLimit::createFromBson(
        BSONElement *pBsonElement,
        const intrusive_ptr<ExpressionContext> &pExpCtx) {
        uassert(15957, "the limit must be specified as a number",
                pBsonElement->isNumber());

        long long limit = pBsonElement->numberLong();
        return DocumentSourceLimit::create(pExpCtx, limit);
    }
}
