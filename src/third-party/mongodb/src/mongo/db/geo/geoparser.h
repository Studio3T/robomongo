/**
*    Copyright (C) 2008-2012 10gen Inc.
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

#pragma once

#include "mongo/db/jsobj.h"
#include <vector>
#include "third_party/s2/s2.h"
#include "mongo/db/geo/shapes.h"

class S2Cap;
class S2Cell;
class S2Polyline;
class S2Polygon;

namespace mongo {
    // This class parses geographic data.
    // It parses a subset of GeoJSON and creates S2 shapes from it.
    // See http://geojson.org/geojson-spec.html for the spec.
    //
    // This class also parses the ad-hoc geo formats that MongoDB introduced.
    //
    // The parseFoo methods that return a bool internally call isFoo and return true
    // if the foo is parsed correctly.
    // The parseFoo methods that do not return a bool assume isFoo is true.
    //
    // We assume that if you're trying to parse something, you know it's valid.
    class GeoParser {
    public:
        // Try to parse GeoJSON, then try legacy format, return true if either succeed.
        // These call the various isPoint and parsePoint methods below.
        // You can just use these bool parsePoint(...) methods.
        static bool parsePoint(const BSONObj &obj, S2Point *out);
        static bool parsePoint(const BSONObj &obj, S2Cell *out);
        static bool parsePoint(const BSONObj &obj, Point *out);
        // Check to see if it's GeoJSON or if it's legacy geo.
        static bool isPoint(const BSONObj &obj);

        static bool isGeoJSONPoint(const BSONObj &obj);
        static void parseGeoJSONPoint(const BSONObj &obj, S2Point *out);
        static void parseGeoJSONPoint(const BSONObj &obj, S2Cell *out);
        static void parseGeoJSONPoint(const BSONObj &obj, Point *out);

        static bool isLegacyPoint(const BSONObj &obj);
        static void parseLegacyPoint(const BSONObj &obj, S2Point *out);
        static void parseLegacyPoint(const BSONObj &obj, Point *out);

        static bool parseLineString(const BSONObj &obj, S2Polyline *out);
        static bool isLineString(const BSONObj &obj);
        static bool isGeoJSONLineString(const BSONObj &obj);
        static void parseGeoJSONLineString(const BSONObj &obj, S2Polyline *out);

        static bool parsePolygon(const BSONObj &obj, S2Polygon *out);
        static bool parsePolygon(const BSONObj &obj, Polygon *out);
        static bool isPolygon(const BSONObj &obj);
        static bool isGeoJSONPolygon(const BSONObj &obj);
        static bool isLegacyPolygon(const BSONObj &obj);
        static void parseGeoJSONPolygon(const BSONObj &obj, S2Polygon *out);
        static void parseLegacyPolygon(const BSONObj &obj, Polygon *out);

        static bool isLegacyBox(const BSONObj &obj);
        static void parseLegacyBox(const BSONObj &obj, Box *out);

        static bool isLegacyCenter(const BSONObj &obj);
        static void parseLegacyCenter(const BSONObj &obj, Circle *out);

        static bool isLegacyCenterSphere(const BSONObj &obj);
        static void parseLegacyCenterSphere(const BSONObj &obj, S2Cap *out);

        // Return true if the CRS field is 1. missing, or 2. is well-formed and
        // has a datum we accept.  Otherwise, return false.
        // NOTE(hk): If this is ever used anywhere but internally, consider
        // returning states: missing, invalid, unknown, ok, etc. -- whatever
        // needed.
        static bool crsIsOK(const BSONObj& obj);
    };
}  // namespace mongo
