/**
*    Copyright (C) 2012 10gen Inc.
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
#include "mongo/db/geo/geoconstants.h"
#include "mongo/db/geo/s2.h"
#include "third_party/s2/s2cell.h"
#include "third_party/s2/s2polyline.h"
#include "third_party/s2/s2polygon.h"
#include "third_party/s2/s2regioncoverer.h"

#pragma once

namespace mongo {

// An enum describing the version of an S2 index.
enum S2IndexVersion {
    // The first version of the S2 index, introduced in MongoDB 2.4.0.  Compatible with MongoDB
    // 2.4.0 and later.  Supports the following GeoJSON objects: Point, LineString, Polygon.
    S2_INDEX_VERSION_1 = 1,

    // The current version of the S2 index, introduced in MongoDB 2.6.0.  Compatible with
    // MongoDB 2.6.0 and later.  Introduced support for the following GeoJSON objects:
    // MultiPoint, MultiLineString, MultiPolygon, GeometryCollection.
    S2_INDEX_VERSION_2 = 2
};

struct S2IndexingParams {
    // Since we take the cartesian product when we generate keys for an insert,
    // we need a cap.
    size_t maxKeysPerInsert;
    // This is really an advisory parameter that we pass to the cover generator.  The
    // finest/coarsest index level determine the required # of cells.
    int maxCellsInCovering;
    // What's the finest grained level that we'll index?  When we query for a point
    // we start at that -- we index nothing finer than this.
    int finestIndexedLevel;
    // And, what's the coarsest?  When we search in larger coverings we know we
    // can stop here -- we index nothing coarser than this.
    int coarsestIndexedLevel;
    // Version of this index (specific to the index type).
    S2IndexVersion indexVersion;

    double radius;

    std::string toString() const {
        std::stringstream ss;
        ss << "maxKeysPerInsert: " << maxKeysPerInsert << std::endl;
        ss << "maxCellsInCovering: " << maxCellsInCovering << std::endl;
        ss << "finestIndexedLevel: " << finestIndexedLevel << std::endl;
        ss << "coarsestIndexedLevel: " << coarsestIndexedLevel << std::endl;
        ss << "indexVersion: " << indexVersion << std::endl;
        return ss.str();
    }

    void configureCoverer(S2RegionCoverer* coverer) const {
        coverer->set_min_level(coarsestIndexedLevel);
        coverer->set_max_level(finestIndexedLevel);
        // This is advisory; the two above are strict.
        coverer->set_max_cells(maxCellsInCovering);
    }
};

}  // namespace mongo
