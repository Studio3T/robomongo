/**
 * Copyright 2011 (c) 10gen Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mongo/pch.h"

#include "mongo/db/pipeline/document_source.h"
#include "mongo/db/pipeline/document.h"

namespace mongo {
    char DocumentSourceGeoNear::geoNearName[] = "$geoNear";
    const char *DocumentSourceGeoNear::getSourceName() const { return geoNearName; }

    DocumentSourceGeoNear::~DocumentSourceGeoNear() {}

    bool DocumentSourceGeoNear::eof() {
        if (!resultsIterator)
            runCommand();

        return !hasCurrent;
    }

    bool DocumentSourceGeoNear::advance() {
        if (!resultsIterator)
            runCommand();

        hasCurrent = resultsIterator->more();
        if (hasCurrent) {
            // each result from the geoNear command is wrapped in a wrapper object with "obj",
            // "dis" and maybe "loc" fields. We want to take the object from "obj" and inject the
            // other fields into it.

            Document result (resultsIterator->next().embeddedObject());
            MutableDocument output (result["obj"].getDocument());
            output.setNestedField(*distanceField, result["dis"]);
            if (includeLocs)
                output.setNestedField(*includeLocs, result["loc"]);

            currentDoc = output.freeze();
        }
        return hasCurrent;
    }

    Document DocumentSourceGeoNear::getCurrent() {
        verify(hasCurrent);
        return currentDoc;
    }

    void DocumentSourceGeoNear::setSource(DocumentSource*) {
        uasserted(16602, "$geoNear is only allowed as the first pipeline stage");
    }

    bool DocumentSourceGeoNear::coalesce(const intrusive_ptr<DocumentSource> &pNextSource) {
        DocumentSourceLimit* limitSrc = dynamic_cast<DocumentSourceLimit*>(pNextSource.get());
        if (limitSrc) {
            limit = min(limit, limitSrc->getLimit());
            return true;
        }

        return false;
    }

    // This command is sent as-is to the shards.
    // On router this becomes a sort by distance (nearest-first) with limit.
    intrusive_ptr<DocumentSource> DocumentSourceGeoNear::getShardSource() { return this; }
    intrusive_ptr<DocumentSource> DocumentSourceGeoNear::getRouterSource() {
        return DocumentSourceSort::create(pExpCtx,
                                          BSON(distanceField->getPath(false) << 1),
                                          limit);
    }

    void DocumentSourceGeoNear::sourceToBson(BSONObjBuilder *pBuilder, bool explain) const {
        BSONObjBuilder geoNear (pBuilder->subobjStart("$geoNear"));

        if (coordsIsArray) {
            geoNear.appendArray("near", coords);
        }
        else {
            geoNear.append("near", coords);
        }

        geoNear.append("distanceField", distanceField->getPath(false)); // not in buildGeoNearCmd
        geoNear.append("limit", limit);

        if (maxDistance > 0)
            geoNear.append("maxDistance", maxDistance);

        geoNear.append("query", query);
        geoNear.append("spherical", spherical);
        geoNear.append("distanceMultiplier", distanceMultiplier);

        if (includeLocs)
            geoNear.append("includeLocs", includeLocs->getPath(false));

        geoNear.append("uniqueDocs", uniqueDocs);

        geoNear.doneFast();
    }

    BSONObj DocumentSourceGeoNear::buildGeoNearCmd(const StringData& collection) const {
        // this is very similar to sourceToBson, but slightly different.
        // differences will be noted.

        BSONObjBuilder geoNear; // not building a subField

        geoNear.append("geoNear", collection); // not in toBson

        if (coordsIsArray) {
            geoNear.appendArray("near", coords);
        }
        else {
            geoNear.append("near", coords);
        }

        geoNear.append("num", limit); // called limit in toBson

        if (maxDistance > 0)
            geoNear.append("maxDistance", maxDistance);

        geoNear.append("query", query);
        geoNear.append("spherical", spherical);
        geoNear.append("distanceMultiplier", distanceMultiplier);

        if (includeLocs)
            geoNear.append("includeLocs", true); // String in toBson

        geoNear.append("uniqueDocs", uniqueDocs);

        return geoNear.obj();
    }

    void DocumentSourceGeoNear::runCommand() {
        massert(16603, "Already ran geoNearCommand",
                !resultsIterator);

        bool ok = client->runCommand(db, buildGeoNearCmd(collection), cmdOutput);
        uassert(16604, "geoNear command failed: " + cmdOutput.toString(),
                ok);

        resultsIterator.reset(new BSONObjIterator(cmdOutput["results"].embeddedObject()));
        advance(); // consumes first result into currentDoc
    }

    intrusive_ptr<DocumentSourceGeoNear> DocumentSourceGeoNear::create(
            const intrusive_ptr<ExpressionContext> &pCtx) {
        return new DocumentSourceGeoNear(pCtx);
    }

    intrusive_ptr<DocumentSource> DocumentSourceGeoNear::createFromBson(
            BSONElement *pBsonElement,
            const intrusive_ptr<ExpressionContext> &pCtx) {
        intrusive_ptr<DocumentSourceGeoNear> out = new DocumentSourceGeoNear(pCtx);
        out->parseOptions(pBsonElement->embeddedObjectUserCheck());
        return out;
    }

    void DocumentSourceGeoNear::parseOptions(BSONObj options) {
        // near and distanceField are required

        uassert(16605, "$geoNear requires a 'near' option as an Array",
                options["near"].isABSONObj()); // Array or Object (Object is deprecated)
        coordsIsArray = options["near"].type() == Array;
        coords = options["near"].embeddedObject().getOwned();

        uassert(16606, "$geoNear requires a 'distanceField' option as a String",
                options["distanceField"].type() == String);
        distanceField.reset(new FieldPath(options["distanceField"].str()));

        // remaining fields are optional

        // num and limit are synonyms
        if (options["limit"].isNumber())
            limit = options["limit"].numberLong();
        if (options["num"].isNumber())
            limit = options["num"].numberLong();

        if (options["maxDistance"].isNumber())
            maxDistance = options["maxDistance"].numberDouble();

        if (options["query"].type() == Object)
            query = options["query"].embeddedObject().getOwned();

        spherical = options["spherical"].trueValue();

        if (options["distanceMultiplier"].isNumber())
            distanceMultiplier = options["distanceMultiplier"].numberDouble();

        if (options.hasField("includeLocs")) {
            uassert(16607, "$geoNear requires that 'includeLocs' option is a String",
                    options["includeLocs"].type() == String);
            includeLocs.reset(new FieldPath(options["includeLocs"].str()));
        }

        uniqueDocs = options["uniqueDocs"].trueValue();
    }

    DocumentSourceGeoNear::DocumentSourceGeoNear(const intrusive_ptr<ExpressionContext> &pExpCtx)
        : SplittableDocumentSource(pExpCtx)
        , coordsIsArray(false)
        , limit(100)
        , maxDistance(-1.0)
        , spherical(false)
        , distanceMultiplier(1.0)
        , uniqueDocs(true)
        , hasCurrent(false)
    {}
}
