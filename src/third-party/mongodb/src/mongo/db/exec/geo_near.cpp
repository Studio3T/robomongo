
/**
 *    Copyright (C) 2014 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kQuery

#include "mongo/db/exec/geo_near.h"

// For s2 search
#include "third_party/s2/s2regionintersection.h"

#include "mongo/base/owned_pointer_vector.h"
#include "mongo/db/exec/index_scan.h"
#include "mongo/db/exec/fetch.h"
#include "mongo/db/exec/working_set_computed_data.h"
#include "mongo/db/geo/geoconstants.h"
#include "mongo/db/geo/geoparser.h"
#include "mongo/db/geo/hash.h"
#include "mongo/db/matcher/expression.h"
#include "mongo/db/query/expression_index.h"
#include "mongo/db/query/expression_index_knobs.h"
#include "mongo/db/index/expression_params.h"
#include "mongo/util/log.h"

#include <boost/scoped_ptr.hpp>
#include <algorithm>

namespace mongo {

using boost::scoped_ptr;
using std::abs;
using std::auto_ptr;

//
// Shared GeoNear search functionality
//

static const double kCircOfEarthInMeters = 2 * M_PI * kRadiusOfEarthInMeters;
static const double kMaxEarthDistanceInMeters = kCircOfEarthInMeters / 2;
static const double kMetersPerDegreeAtEquator = kCircOfEarthInMeters / 360;

namespace {

/**
 * Structure that holds BSON addresses (BSONElements) and the corresponding geometry parsed
 * at those locations.
 * Used to separate the parsing of geometries from a BSONObj (which must stay in scope) from
 * the computation over those geometries.
 * TODO: Merge with 2D/2DSphere key extraction?
 */
struct StoredGeometry {
    static StoredGeometry* parseFrom(const BSONElement& element) {
        if (!element.isABSONObj())
            return NULL;

        auto_ptr<StoredGeometry> stored(new StoredGeometry);
        if (!stored->geometry.parseFromStorage(element).isOK())
            return NULL;
        stored->element = element;
        return stored.release();
    }

    BSONElement element;
    GeometryContainer geometry;
};
}

/**
 * Find and parse all geometry elements on the appropriate field path from the document.
 */
static void extractGeometries(const BSONObj& doc,
                              const string& path,
                              vector<StoredGeometry*>* geometries) {
    BSONElementSet geomElements;
    // NOTE: Annoyingly, we cannot just expand arrays b/c single 2d points are arrays, we need
    // to manually expand all results to check if they are geometries
    doc.getFieldsDotted(path, geomElements, false /* expand arrays */);

    for (BSONElementSet::iterator it = geomElements.begin(); it != geomElements.end(); ++it) {
        const BSONElement& el = *it;
        auto_ptr<StoredGeometry> stored(StoredGeometry::parseFrom(el));

        if (stored.get()) {
            // Valid geometry element
            geometries->push_back(stored.release());
        } else if (el.type() == Array) {
            // Many geometries may be in an array
            BSONObjIterator arrIt(el.Obj());
            while (arrIt.more()) {
                const BSONElement nextEl = arrIt.next();
                stored.reset(StoredGeometry::parseFrom(nextEl));

                if (stored.get()) {
                    // Valid geometry element
                    geometries->push_back(stored.release());
                } else {
                    warning() << "geoNear stage read non-geometry element " << nextEl.toString()
                              << " in array " << el.toString();
                }
            }
        } else {
            warning() << "geoNear stage read non-geometry element " << el.toString();
        }
    }
}

static StatusWith<double> computeGeoNearDistance(const GeoNearParams& nearParams,
                                                 WorkingSetMember* member) {
    //
    // Generic GeoNear distance computation
    // Distances are computed by projecting the stored geometry into the query CRS, and
    // computing distance in that CRS.
    //

    // Must have an object in order to get geometry out of it.
    invariant(member->hasObj());

    CRS queryCRS = nearParams.nearQuery->centroid->crs;

    // Extract all the geometries out of this document for the near query
    OwnedPointerVector<StoredGeometry> geometriesOwned;
    vector<StoredGeometry*>& geometries = geometriesOwned.mutableVector();
    extractGeometries(member->obj.value(), nearParams.nearQuery->field, &geometries);

    // Compute the minimum distance of all the geometries in the document
    double minDistance = -1;
    BSONObj minDistanceObj;
    for (vector<StoredGeometry*>::iterator it = geometries.begin(); it != geometries.end(); ++it) {
        StoredGeometry& stored = **it;

        // NOTE: A stored document with STRICT_SPHERE CRS is treated as a malformed document
        // and ignored. Since GeoNear requires an index, there's no stored STRICT_SPHERE shape.
        // So we don't check it here.

        // NOTE: For now, we're sure that if we get this far in the query we'll have an
        // appropriate index which validates the type of geometry we're pulling back here.
        // TODO: It may make sense to change our semantics and, by default, only return
        // shapes in the same CRS from $geoNear.
        if (!stored.geometry.supportsProject(queryCRS))
            continue;
        stored.geometry.projectInto(queryCRS);

        double nextDistance = stored.geometry.minDistance(*nearParams.nearQuery->centroid);

        if (minDistance < 0 || nextDistance < minDistance) {
            minDistance = nextDistance;
            minDistanceObj = stored.element.Obj();
        }
    }

    if (minDistance < 0) {
        // No distance to report
        return StatusWith<double>(-1);
    }

    if (nearParams.addDistMeta) {
        if (nearParams.nearQuery->unitsAreRadians) {
            // Hack for nearSphere
            // TODO: Remove nearSphere?
            invariant(SPHERE == queryCRS);
            member->addComputed(new GeoDistanceComputedData(minDistance / kRadiusOfEarthInMeters));
        } else {
            member->addComputed(new GeoDistanceComputedData(minDistance));
        }
    }

    if (nearParams.addPointMeta) {
        member->addComputed(new GeoNearPointComputedData(minDistanceObj));
    }

    return StatusWith<double>(minDistance);
}

static R2Annulus geoNearDistanceBounds(const GeoNearExpression& query) {
    const CRS queryCRS = query.centroid->crs;

    if (FLAT == queryCRS) {
        return R2Annulus(query.centroid->oldPoint, query.minDistance, query.maxDistance);
    }

    invariant(SPHERE == queryCRS);

    // TODO: Tighten this up a bit by making a CRS for "sphere with radians"
    double minDistance = query.minDistance;
    double maxDistance = query.maxDistance;

    if (query.unitsAreRadians) {
        // Our input bounds are in radians, convert to meters since the query CRS is actually
        // SPHERE.  We'll convert back to radians on outputting distances.
        minDistance *= kRadiusOfEarthInMeters;
        maxDistance *= kRadiusOfEarthInMeters;
    }

    // GOTCHA: oldPoint is a misnomer - it is the original point data and is in the correct
    // CRS.  We must not try to derive the original point from the spherical S2Point generated
    // as an optimization - the mapping is not 1->1 - [-180, 0] and [180, 0] map to the same
    // place.
    // TODO: Wrapping behavior should not depend on the index, which would make $near code
    // insensitive to which direction we explore the index in.
    return R2Annulus(query.centroid->oldPoint,
                     min(minDistance, kMaxEarthDistanceInMeters),
                     min(maxDistance, kMaxEarthDistanceInMeters));
}

//
// GeoNear2DStage
//

static R2Annulus twoDDistanceBounds(const GeoNearParams& nearParams,
                                    const IndexDescriptor* twoDIndex) {
    R2Annulus fullBounds = geoNearDistanceBounds(*nearParams.nearQuery);
    const CRS queryCRS = nearParams.nearQuery->centroid->crs;

    if (FLAT == queryCRS) {
        // Reset the full bounds based on our index bounds
        GeoHashConverter::Parameters hashParams;
        Status status = GeoHashConverter::parseParameters(twoDIndex->infoObj(), &hashParams);
        invariant(status.isOK());  // The index status should always be valid

        // The biggest distance possible in this indexed collection is the diagonal of the
        // square indexed region.
        const double sqrt2Approx = 1.5;
        const double diagonalDist = sqrt2Approx * (hashParams.max - hashParams.min);

        fullBounds = R2Annulus(
            fullBounds.center(), fullBounds.getInner(), min(fullBounds.getOuter(), diagonalDist));
    } else {
        // Spherical queries have upper bounds set by the earth - no-op
        // TODO: Wrapping errors would creep in here if nearSphere wasn't defined to not wrap
        invariant(SPHERE == queryCRS);
        invariant(!nearParams.nearQuery->isWrappingQuery);
    }

    return fullBounds;
}

class GeoNear2DStage::DensityEstimator {
public:
    DensityEstimator(const IndexDescriptor* twoDindex, const GeoNearParams* nearParams)
        : _twoDIndex(twoDindex), _nearParams(nearParams), _currentLevel(0) {
        GeoHashConverter::Parameters hashParams;
        Status status = GeoHashConverter::parseParameters(_twoDIndex->infoObj(), &hashParams);
        // The index status should always be valid.
        invariant(status.isOK());

        _converter.reset(new GeoHashConverter(hashParams));
        _centroidCell = _converter->hash(_nearParams->nearQuery->centroid->oldPoint);

        // Since appendVertexNeighbors(level, output) requires level < hash.getBits(),
        // we have to start to find documents at most GeoHash::kMaxBits - 1. Thus the finest
        // search area is 16 * finest cell area at GeoHash::kMaxBits.
        _currentLevel = std::max(0u, hashParams.bits - 1u);
    }

    PlanStage::StageState work(OperationContext* txn,
                               WorkingSet* workingSet,
                               Collection* collection,
                               double* estimatedDistance);

    void saveState();
    void restoreState(OperationContext* txn);
    void invalidate(OperationContext* txn, const RecordId& dl, InvalidationType type);

private:
    void buildIndexScan(OperationContext* txn, WorkingSet* workingSet, Collection* collection);

    const IndexDescriptor* _twoDIndex;  // Not owned here.
    const GeoNearParams* _nearParams;   // Not owned here.
    scoped_ptr<IndexScan> _indexScan;
    scoped_ptr<GeoHashConverter> _converter;
    GeoHash _centroidCell;
    unsigned _currentLevel;
};

// Initialize the internal states
void GeoNear2DStage::DensityEstimator::buildIndexScan(OperationContext* txn,
                                                      WorkingSet* workingSet,
                                                      Collection* collection) {
    IndexScanParams scanParams;
    scanParams.descriptor = _twoDIndex;
    scanParams.direction = 1;
    scanParams.doNotDedup = true;

    // Scan bounds on 2D indexes are only over the 2D field - other bounds aren't applicable.
    // This is handled in query planning.
    scanParams.bounds = _nearParams->baseBounds;

    // The "2d" field is always the first in the index
    const string twoDFieldName = _nearParams->nearQuery->field;
    const int twoDFieldPosition = 0;

    // Construct index intervals used by this stage
    OrderedIntervalList oil;
    oil.name = scanParams.bounds.fields[twoDFieldPosition].name;

    vector<GeoHash> neighbors;
    // Return the neighbors of closest vertex to this cell at the given level.
    _centroidCell.appendVertexNeighbors(_currentLevel, &neighbors);
    std::sort(neighbors.begin(), neighbors.end());

    for (vector<GeoHash>::const_iterator it = neighbors.begin(); it != neighbors.end(); it++) {
        mongo::BSONObjBuilder builder;
        it->appendHashMin(&builder, "");
        it->appendHashMax(&builder, "");
        oil.intervals.push_back(IndexBoundsBuilder::makeRangeInterval(builder.obj(), true, true));
    }

    invariant(oil.isValidFor(1));

    // Intersect the $near bounds we just generated into the bounds we have for anything else
    // in the scan (i.e. $within)
    IndexBoundsBuilder::intersectize(oil, &scanParams.bounds.fields[twoDFieldPosition]);

    _indexScan.reset(new IndexScan(txn, scanParams, workingSet, NULL));
}

// Return IS_EOF is we find a document in it's ancestor cells and set estimated distance
// from the nearest document.
PlanStage::StageState GeoNear2DStage::DensityEstimator::work(OperationContext* txn,
                                                             WorkingSet* workingSet,
                                                             Collection* collection,
                                                             double* estimatedDistance) {
    if (!_indexScan) {
        // Setup index scan stage for current level.
        buildIndexScan(txn, workingSet, collection);
    }

    WorkingSetID workingSetID;
    PlanStage::StageState state = _indexScan->work(&workingSetID);

    if (state == PlanStage::IS_EOF) {
        // We ran through the neighbors but found nothing.
        if (_currentLevel > 0u) {
            // Advance to the next level and search again.
            _currentLevel--;
            // Reset index scan for the next level.
            _indexScan.reset(NULL);
            return PlanStage::NEED_TIME;
        }

        // We are already at the top level.
        *estimatedDistance = _converter->sizeEdge(_currentLevel);
        return PlanStage::IS_EOF;
    } else if (state == PlanStage::ADVANCED) {
        // Found a document at current level.
        *estimatedDistance = _converter->sizeEdge(_currentLevel);
        // Clean up working set.
        workingSet->free(workingSetID);
        return PlanStage::IS_EOF;
    }

    invariant(state != NEED_FETCH);

    // Propagate NEED_TIME or errors
    return state;
}

void GeoNear2DStage::DensityEstimator::saveState() {
    if (_indexScan) {
        _indexScan->saveState();
    }
}

void GeoNear2DStage::DensityEstimator::restoreState(OperationContext* txn) {
    if (_indexScan) {
        _indexScan->restoreState(txn);
    }
}

void GeoNear2DStage::DensityEstimator::invalidate(OperationContext* txn,
                                                  const RecordId& dl,
                                                  InvalidationType type) {
    if (_indexScan) {
        _indexScan->invalidate(txn, dl, type);
    }
}


PlanStage::StageState GeoNear2DStage::initialize(OperationContext* txn,
                                                 WorkingSet* workingSet,
                                                 Collection* collection) {
    if (!_densityEstimator) {
        _densityEstimator.reset(new DensityEstimator(_twoDIndex, &_nearParams));
    }

    double estimatedDistance;
    PlanStage::StageState state =
        _densityEstimator->work(txn, workingSet, collection, &estimatedDistance);

    if (state == PlanStage::IS_EOF) {
        // 2d index only works with legacy points as centroid. $nearSphere will project
        // the point into SPHERE CRS and calculate distance based on that.
        // STRICT_SPHERE is impossible here, as GeoJSON centroid is not allowed for 2d index.

        // Estimator finished its work, we need to finish initialization too.
        if (SPHERE == _nearParams.nearQuery->centroid->crs) {
            // Estimated distance is in degrees, convert it to meters.
            _boundsIncrement = deg2rad(estimatedDistance) * kRadiusOfEarthInMeters * 3;
            // Limit boundsIncrement to ~20KM, so that the first circle won't be too aggressive.
            _boundsIncrement = std::min(_boundsIncrement, kMaxEarthDistanceInMeters / 1000.0);
        } else {
            // We expand the radius by 3 times to give a reasonable starting search area.
            // Assume points are distributed evenly. X is the edge size of cells at whose
            // level we found a document in 4 neighbors. Thus the closest point is at least
            // X/2 far from the centroid. The distance between two points is at least X.
            // The area of Pi * (3X)^2 ~= 28 * X^2 will cover dozens of points at most.
            // We'll explore the space with exponentially increasing radius if this guess is
            // too small, so starting from a conservative initial radius doesn't hurt.

            _boundsIncrement = 3 * estimatedDistance;
        }
        invariant(_boundsIncrement > 0.0);

        // Clean up
        _densityEstimator.reset(NULL);
    }

    return state;
}

static const string kTwoDIndexNearStage("GEO_NEAR_2D");

GeoNear2DStage::GeoNear2DStage(const GeoNearParams& nearParams,
                               OperationContext* txn,
                               WorkingSet* workingSet,
                               Collection* collection,
                               IndexDescriptor* twoDIndex)
    : NearStage(txn,
                workingSet,
                collection,
                new PlanStageStats(CommonStats(kTwoDIndexNearStage.c_str()), STAGE_GEO_NEAR_2D)),
      _nearParams(nearParams),
      _twoDIndex(twoDIndex),
      _fullBounds(twoDDistanceBounds(nearParams, twoDIndex)),
      _currBounds(_fullBounds.center(), -1, _fullBounds.getInner()),
      _boundsIncrement(0.0) {
    getNearStats()->keyPattern = twoDIndex->keyPattern();
    getNearStats()->indexName = twoDIndex->indexName();
}

GeoNear2DStage::~GeoNear2DStage() {}

void GeoNear2DStage::finishSaveState() {
    if (_densityEstimator) {
        _densityEstimator->saveState();
    }
}

void GeoNear2DStage::finishRestoreState(OperationContext* txn) {
    if (_densityEstimator) {
        _densityEstimator->restoreState(txn);
    }
}

void GeoNear2DStage::finishInvalidate(OperationContext* txn,
                                      const RecordId& dl,
                                      InvalidationType type) {
    if (_densityEstimator) {
        _densityEstimator->invalidate(txn, dl, type);
    }
}

namespace {

/**
 * Expression which checks whether a legacy 2D index point is contained within our near
 * search annulus.  See nextInterval() below for more discussion.
 * TODO: Make this a standard type of GEO match expression
 */
class TwoDPtInAnnulusExpression : public LeafMatchExpression {
public:
    TwoDPtInAnnulusExpression(const R2Annulus& annulus, const StringData& twoDPath)
        : LeafMatchExpression(INTERNAL_2D_POINT_IN_ANNULUS), _annulus(annulus) {
        initPath(twoDPath);
    }

    virtual ~TwoDPtInAnnulusExpression() {}

    virtual void toBSON(BSONObjBuilder* out) const {
        out->append("TwoDPtInAnnulusExpression", true);
    }

    virtual bool matchesSingleElement(const BSONElement& e) const {
        if (!e.isABSONObj())
            return false;

        PointWithCRS point;
        if (!GeoParser::parseStoredPoint(e, &point).isOK())
            return false;

        return _annulus.contains(point.oldPoint);
    }

    //
    // These won't be called.
    //

    virtual void debugString(StringBuilder& debug, int level = 0) const {
        invariant(false);
    }

    virtual bool equivalent(const MatchExpression* other) const {
        invariant(false);
        return false;
    }

    virtual LeafMatchExpression* shallowClone() const {
        invariant(false);
        return NULL;
    }

private:
    R2Annulus _annulus;
};

/**
 * Expression which checks whether a 2D key for a point (2D hash) intersects our search
 * region.  The search region may have been formed by more granular hashes.
 */
class TwoDKeyInRegionExpression : public LeafMatchExpression {
public:
    TwoDKeyInRegionExpression(R2Region* region,
                              const GeoHashConverter::Parameters& hashParams,
                              const StringData& twoDKeyPath)
        : LeafMatchExpression(INTERNAL_2D_KEY_IN_REGION), _region(region), _unhasher(hashParams) {
        initPath(twoDKeyPath);
    }

    virtual ~TwoDKeyInRegionExpression() {}

    virtual void toBSON(BSONObjBuilder* out) const {
        out->append("TwoDKeyInRegionExpression", true);
    }

    virtual bool matchesSingleElement(const BSONElement& e) const {
        // Something has gone terribly wrong if this doesn't hold.
        invariant(BinData == e.type());
        return !_region->fastDisjoint(_unhasher.unhashToBoxCovering(_unhasher.hash(e)));
    }

    //
    // These won't be called.
    //

    virtual void debugString(StringBuilder& debug, int level = 0) const {
        invariant(false);
    }

    virtual bool equivalent(const MatchExpression* other) const {
        invariant(false);
        return true;
    }

    virtual MatchExpression* shallowClone() const {
        invariant(false);
        return NULL;
    }

private:
    const scoped_ptr<R2Region> _region;
    const GeoHashConverter _unhasher;
};

// Helper class to maintain ownership of a match expression alongside an index scan
class IndexScanWithMatch : public IndexScan {
public:
    IndexScanWithMatch(OperationContext* txn,
                       const IndexScanParams& params,
                       WorkingSet* workingSet,
                       MatchExpression* filter)
        : IndexScan(txn, params, workingSet, filter), _matcher(filter) {}

    virtual ~IndexScanWithMatch() {}

private:
    // Owns matcher
    const scoped_ptr<MatchExpression> _matcher;
};

// Helper class to maintain ownership of a match expression alongside an index scan
class FetchStageWithMatch : public FetchStage {
public:
    FetchStageWithMatch(OperationContext* txn,
                        WorkingSet* ws,
                        PlanStage* child,
                        MatchExpression* filter,
                        const Collection* collection)
        : FetchStage(txn, ws, child, filter, collection), _matcher(filter) {}

    virtual ~FetchStageWithMatch() {}

private:
    // Owns matcher
    const scoped_ptr<MatchExpression> _matcher;
};
}

static double min2DBoundsIncrement(const GeoNearExpression& query, IndexDescriptor* twoDIndex) {
    GeoHashConverter::Parameters hashParams;
    Status status = GeoHashConverter::parseParameters(twoDIndex->infoObj(), &hashParams);
    invariant(status.isOK());  // The index status should always be valid
    GeoHashConverter hasher(hashParams);

    // The hasher error is the diagonal of a 2D hash region - it's generally not helpful
    // to change region size such that a search radius is smaller than the 2D hash region
    // max radius.  This is slightly conservative for now (box diagonal vs circle radius).
    double minBoundsIncrement = hasher.getError() / 2;

    const CRS queryCRS = query.centroid->crs;
    if (FLAT == queryCRS)
        return minBoundsIncrement;

    invariant(SPHERE == queryCRS);

    // If this is a spherical query, units are in meters - this is just a heuristic
    return minBoundsIncrement * kMetersPerDegreeAtEquator;
}

static R2Annulus projectBoundsToTwoDDegrees(R2Annulus sphereBounds) {
    const double outerDegrees = rad2deg(sphereBounds.getOuter() / kRadiusOfEarthInMeters);
    const double innerDegrees = rad2deg(sphereBounds.getInner() / kRadiusOfEarthInMeters);
    const double maxErrorDegrees = computeXScanDistance(sphereBounds.center().y, outerDegrees);

    return R2Annulus(sphereBounds.center(),
                     max(0.0, innerDegrees - maxErrorDegrees),
                     outerDegrees + maxErrorDegrees);
}

StatusWith<NearStage::CoveredInterval*>  //
    GeoNear2DStage::nextInterval(OperationContext* txn,
                                 WorkingSet* workingSet,
                                 Collection* collection) {
    // The search is finished if we searched at least once and all the way to the edge
    if (_currBounds.getInner() >= 0 && _currBounds.getOuter() == _fullBounds.getOuter()) {
        return StatusWith<CoveredInterval*>(NULL);
    }

    //
    // Setup the next interval
    //

    const NearStats* stats = getNearStats();

    if (!stats->intervalStats.empty()) {
        const IntervalStats& lastIntervalStats = stats->intervalStats.back();

        // TODO: Generally we want small numbers of results fast, then larger numbers later
        if (lastIntervalStats.numResultsBuffered < 300)
            _boundsIncrement *= 2;
        else if (lastIntervalStats.numResultsBuffered > 600)
            _boundsIncrement /= 2;
    }

    _boundsIncrement =
        max(_boundsIncrement, min2DBoundsIncrement(*_nearParams.nearQuery, _twoDIndex));

    R2Annulus nextBounds(_currBounds.center(),
                         _currBounds.getOuter(),
                         min(_currBounds.getOuter() + _boundsIncrement, _fullBounds.getOuter()));

    const bool isLastInterval = (nextBounds.getOuter() == _fullBounds.getOuter());
    _currBounds = nextBounds;

    //
    // Get a covering region for this interval
    //

    const CRS queryCRS = _nearParams.nearQuery->centroid->crs;

    auto_ptr<R2Region> coverRegion;

    if (FLAT == queryCRS) {
        // NOTE: Due to floating point math issues, FLAT searches of a 2D index need to treat
        // containment and distance separately.
        // Ex: (distance) 54.001 - 54 > 0.001, but (containment) 54 + 0.001 <= 54.001
        // The idea is that a $near search with bounds is really a $within search, sorted by
        // distance.  We attach a custom $within : annulus matcher to do the $within search,
        // and adjust max/min bounds slightly since, as above, containment does not mean the
        // distance calculation won't slightly overflow the boundary.
        //
        // The code below adjusts:
        // 1) Overall min/max bounds of the generated distance intervals to be more inclusive
        // 2) Bounds of the interval covering to be more inclusive
        // ... and later on we add the custom $within : annulus matcher.
        //
        // IMPORTANT: The *internal* interval distance bounds are *exact thresholds* - these
        // should not be adjusted.
        // TODO: Maybe integrate annuluses as a standard shape, and literally transform $near
        // internally into a $within query with $near just as sort.

        // Compute the maximum axis-aligned distance error
        const double epsilon = std::numeric_limits<double>::epsilon() *
            (max(abs(_fullBounds.center().x), abs(_fullBounds.center().y)) +
             _fullBounds.getOuter());

        if (nextBounds.getInner() > 0 && nextBounds.getInner() == _fullBounds.getInner()) {
            nextBounds = R2Annulus(nextBounds.center(),
                                   max(0.0, nextBounds.getInner() - epsilon),
                                   nextBounds.getOuter());
        }

        if (nextBounds.getOuter() > 0 && nextBounds.getOuter() == _fullBounds.getOuter()) {
            // We're at the max bound of the search, adjust interval maximum
            nextBounds = R2Annulus(
                nextBounds.center(), nextBounds.getInner(), nextBounds.getOuter() + epsilon);
        }

        // *Always* adjust the covering bounds to be more inclusive
        coverRegion.reset(new R2Annulus(nextBounds.center(),
                                        max(0.0, nextBounds.getInner() - epsilon),
                                        nextBounds.getOuter() + epsilon));
    } else {
        invariant(SPHERE == queryCRS);
        // TODO: As above, make this consistent with $within : $centerSphere

        // Our intervals aren't in the same CRS as our index, so we need to adjust them
        coverRegion.reset(new R2Annulus(projectBoundsToTwoDDegrees(nextBounds)));
    }

    //
    // Setup the stages for this interval
    //

    IndexScanParams scanParams;
    scanParams.descriptor = _twoDIndex;
    scanParams.direction = 1;
    // We use a filter on the key.  The filter rejects keys that don't intersect with the
    // annulus.  An object that is in the annulus might have a key that's not in it and a key
    // that's in it.  As such we can't just look at one key per object.
    //
    // This does force us to do our own deduping of results, though.
    scanParams.doNotDedup = true;

    // Scan bounds on 2D indexes are only over the 2D field - other bounds aren't applicable.
    // This is handled in query planning.
    scanParams.bounds = _nearParams.baseBounds;

    // The "2d" field is always the first in the index
    const string twoDFieldName = _nearParams.nearQuery->field;
    const int twoDFieldPosition = 0;

    OrderedIntervalList coveredIntervals;
    coveredIntervals.name = scanParams.bounds.fields[twoDFieldPosition].name;

    ExpressionMapping::cover2d(*coverRegion,
                               _twoDIndex->infoObj(),
                               internalGeoNearQuery2DMaxCoveringCells,
                               &coveredIntervals);

    // Intersect the $near bounds we just generated into the bounds we have for anything else
    // in the scan (i.e. $within)
    IndexBoundsBuilder::intersectize(coveredIntervals,
                                     &scanParams.bounds.fields[twoDFieldPosition]);

    // These parameters are stored by the index, and so must be ok
    GeoHashConverter::Parameters hashParams;
    GeoHashConverter::parseParameters(_twoDIndex->infoObj(), &hashParams);

    MatchExpression* keyMatcher =
        new TwoDKeyInRegionExpression(coverRegion.release(), hashParams, twoDFieldName);

    // 2D indexes support covered search over additional fields they contain
    // TODO: Don't need to clone, can just attach to custom matcher above
    if (_nearParams.filter) {
        AndMatchExpression* andMatcher = new AndMatchExpression();
        andMatcher->add(keyMatcher);
        andMatcher->add(_nearParams.filter->shallowClone());
        keyMatcher = andMatcher;
    }

    // IndexScanWithMatch owns the matcher
    IndexScan* scan = new IndexScanWithMatch(txn, scanParams, workingSet, keyMatcher);

    MatchExpression* docMatcher = NULL;

    // FLAT searches need to add an additional annulus $within matcher, see above
    if (FLAT == queryCRS) {
        docMatcher = new TwoDPtInAnnulusExpression(_fullBounds, twoDFieldName);
    }

    // FetchStage owns index scan
    FetchStage* fetcher(new FetchStageWithMatch(txn, workingSet, scan, docMatcher, collection));

    return StatusWith<CoveredInterval*>(new CoveredInterval(
        fetcher, true, nextBounds.getInner(), nextBounds.getOuter(), isLastInterval));
}

StatusWith<double> GeoNear2DStage::computeDistance(WorkingSetMember* member) {
    return computeGeoNearDistance(_nearParams, member);
}

//
// GeoNear2DSphereStage
//

static int getFieldPosition(const IndexDescriptor* index, const string& fieldName) {
    int fieldPosition = 0;

    BSONObjIterator specIt(index->keyPattern());
    while (specIt.more()) {
        if (specIt.next().fieldName() == fieldName) {
            break;
        }
        ++fieldPosition;
    }

    if (fieldPosition == index->keyPattern().nFields())
        return -1;

    return fieldPosition;
}

static const string kS2IndexNearStage("GEO_NEAR_2DSPHERE");

GeoNear2DSphereStage::GeoNear2DSphereStage(const GeoNearParams& nearParams,
                                           OperationContext* txn,
                                           WorkingSet* workingSet,
                                           Collection* collection,
                                           IndexDescriptor* s2Index)
    : NearStage(
          txn,
          workingSet,
          collection,
          new PlanStageStats(CommonStats(kS2IndexNearStage.c_str()), STAGE_GEO_NEAR_2DSPHERE)),
      _nearParams(nearParams),
      _s2Index(s2Index),
      _fullBounds(geoNearDistanceBounds(*nearParams.nearQuery)),
      _currBounds(_fullBounds.center(), -1, _fullBounds.getInner()),
      _boundsIncrement(0.0) {
    getNearStats()->keyPattern = s2Index->keyPattern();
    getNearStats()->indexName = s2Index->indexName();
}

GeoNear2DSphereStage::~GeoNear2DSphereStage() {}

namespace {

S2Region* buildS2Region(const R2Annulus& sphereBounds) {
    // Internal bounds come in SPHERE CRS units
    // i.e. center is lon/lat, inner/outer are in meters
    S2LatLng latLng = S2LatLng::FromDegrees(sphereBounds.center().y, sphereBounds.center().x);

    vector<S2Region*> regions;

    S2Cap innerCap = S2Cap::FromAxisAngle(
        latLng.ToPoint(), S1Angle::Radians(sphereBounds.getInner() / kRadiusOfEarthInMeters));
    innerCap = innerCap.Complement();
    regions.push_back(new S2Cap(innerCap));

    // We only need to max bound if this is not a full search of the Earth
    // Using the constant here is important since we use the min of kMaxEarthDistance
    // and the actual bounds passed in to set up the search area.
    if (sphereBounds.getOuter() < kMaxEarthDistanceInMeters) {
        S2Cap outerCap = S2Cap::FromAxisAngle(
            latLng.ToPoint(), S1Angle::Radians(sphereBounds.getOuter() / kRadiusOfEarthInMeters));
        regions.push_back(new S2Cap(outerCap));
    }

    // Takes ownership of caps
    return new S2RegionIntersection(&regions);
}

/**
 * Expression which checks whether a 2DSphere key for a point (S2 hash) intersects our
 * search region.  The search region may have been formed by more granular hashes.
 */
class TwoDSphereKeyInRegionExpression : public LeafMatchExpression {
public:
    TwoDSphereKeyInRegionExpression(const R2Annulus& bounds, StringData twoDSpherePath)
        : LeafMatchExpression(INTERNAL_2DSPHERE_KEY_IN_REGION), _region(buildS2Region(bounds)) {
        initPath(twoDSpherePath);
    }

    virtual ~TwoDSphereKeyInRegionExpression() {}

    virtual void toBSON(BSONObjBuilder* out) const {
        out->append("TwoDSphereKeyInRegionExpression", true);
    }

    virtual bool matchesSingleElement(const BSONElement& e) const {
        // Something has gone terribly wrong if this doesn't hold.
        invariant(String == e.type());
        S2Cell keyCell = S2Cell(S2CellId::FromString(e.str()));
        return _region->MayIntersect(keyCell);
    }

    const S2Region& getRegion() {
        return *_region;
    }

    //
    // These won't be called.
    //

    virtual void debugString(StringBuilder& debug, int level = 0) const {
        invariant(false);
    }

    virtual bool equivalent(const MatchExpression* other) const {
        invariant(false);
        return true;
    }

    virtual MatchExpression* shallowClone() const {
        invariant(false);
        return NULL;
    }

private:
    const scoped_ptr<S2Region> _region;
};
}

// Estimate the density of data by search the nearest cells level by level around center.
class GeoNear2DSphereStage::DensityEstimator {
public:
    DensityEstimator(const IndexDescriptor* s2Index, const GeoNearParams* nearParams)
        : _s2Index(s2Index), _nearParams(nearParams), _currentLevel(0) {
        S2IndexingParams params;
        ExpressionParams::parse2dsphereParams(_s2Index->infoObj(), &params);
        // Since cellId.AppendVertexNeighbors(level, output) requires level < cellId.level(),
        // we have to start to find documents at most S2::kMaxCellLevel - 1. Thus the finest
        // search area is 16 * finest cell area at S2::kMaxCellLevel, which is less than
        // (1.4 inch X 1.4 inch) on the earth.
        _currentLevel = std::max(0, params.finestIndexedLevel - 1);
    }

    // Search for a document in neighbors at current level.
    // Return IS_EOF is such document exists and set the estimated distance to the nearest doc.
    PlanStage::StageState work(OperationContext* txn,
                               WorkingSet* workingSet,
                               Collection* collection,
                               double* estimatedDistance);

    void saveState();
    void restoreState(OperationContext* txn);
    void invalidate(OperationContext* txn, const RecordId& dl, InvalidationType type);

private:
    void buildIndexScan(OperationContext* txn, WorkingSet* workingSet, Collection* collection);

    const IndexDescriptor* _s2Index;   // Not owned here.
    const GeoNearParams* _nearParams;  // Not owned here.
    int _currentLevel;
    scoped_ptr<IndexScan> _indexScan;
};

// Setup the index scan stage for neighbors at this level.
void GeoNear2DSphereStage::DensityEstimator::buildIndexScan(OperationContext* txn,
                                                            WorkingSet* workingSet,
                                                            Collection* collection) {
    IndexScanParams scanParams;
    scanParams.descriptor = _s2Index;
    scanParams.direction = 1;
    scanParams.doNotDedup = true;
    scanParams.bounds = _nearParams->baseBounds;

    // Because the planner doesn't yet set up 2D index bounds, do it ourselves here
    const string s2Field = _nearParams->nearQuery->field;
    const int s2FieldPosition = getFieldPosition(_s2Index, s2Field);
    OrderedIntervalList* coveredIntervals = &scanParams.bounds.fields[s2FieldPosition];
    coveredIntervals->intervals.clear();

    // Find 4 neighbors (3 neighbors at face vertex) at current level.
    const S2CellId& centerId = _nearParams->nearQuery->centroid->cell.id();
    vector<S2CellId> neighbors;

    // The search area expands 4X each time.
    // Return the neighbors of closest vertex to this cell at the given level.
    invariant(_currentLevel < centerId.level());
    centerId.AppendVertexNeighbors(_currentLevel, &neighbors);

    // Convert S2CellId to string and sort
    vector<string> neighborKeys;
    for (vector<S2CellId>::const_iterator it = neighbors.begin(); it != neighbors.end(); it++) {
        neighborKeys.push_back(it->toString());
    }
    std::sort(neighborKeys.begin(), neighborKeys.end());

    for (vector<string>::const_iterator it = neighborKeys.begin(); it != neighborKeys.end(); it++) {
        // construct interval [*it, end) for this cell.
        std::string end = *it;
        end[end.size() - 1]++;
        coveredIntervals->intervals.push_back(
            IndexBoundsBuilder::makeRangeInterval(*it, end, true, false));
    }

    invariant(coveredIntervals->isValidFor(1));

    // Index scan
    _indexScan.reset(new IndexScan(txn, scanParams, workingSet, NULL));
}

PlanStage::StageState GeoNear2DSphereStage::DensityEstimator::work(OperationContext* txn,
                                                                   WorkingSet* workingSet,
                                                                   Collection* collection,
                                                                   double* estimatedDistance) {
    if (!_indexScan) {
        // Setup index scan stage for current level.
        buildIndexScan(txn, workingSet, collection);
    }

    WorkingSetID workingSetID;
    PlanStage::StageState state = _indexScan->work(&workingSetID);

    if (state == PlanStage::IS_EOF) {
        // We ran through the neighbors but found nothing.
        if (_currentLevel > 0) {
            // Advance to the next level and search again.
            _currentLevel--;
            // Reset index scan for the next level.
            _indexScan.reset(NULL);
            return PlanStage::NEED_TIME;
        }

        // We are already at the top level.
        *estimatedDistance = S2::kAvgEdge.GetValue(_currentLevel) * kRadiusOfEarthInMeters;
        return PlanStage::IS_EOF;
    } else if (state == PlanStage::ADVANCED) {
        // We found something!
        *estimatedDistance = S2::kAvgEdge.GetValue(_currentLevel) * kRadiusOfEarthInMeters;
        // Clean up working set.
        workingSet->free(workingSetID);
        return PlanStage::IS_EOF;
    }

    invariant(state != NEED_FETCH);

    // Propagate NEED_TIME or errors
    return state;
}

void GeoNear2DSphereStage::DensityEstimator::saveState() {
    if (_indexScan) {
        _indexScan->saveState();
    }
}

void GeoNear2DSphereStage::DensityEstimator::restoreState(OperationContext* txn) {
    if (_indexScan) {
        _indexScan->restoreState(txn);
    }
}

void GeoNear2DSphereStage::DensityEstimator::invalidate(OperationContext* txn,
                                                        const RecordId& dl,
                                                        InvalidationType type) {
    if (_indexScan) {
        _indexScan->invalidate(txn, dl, type);
    }
}


PlanStage::StageState GeoNear2DSphereStage::initialize(OperationContext* txn,
                                                       WorkingSet* workingSet,
                                                       Collection* collection) {
    if (!_densityEstimator) {
        _densityEstimator.reset(new DensityEstimator(_s2Index, &_nearParams));
    }

    double estimatedDistance;
    PlanStage::StageState state =
        _densityEstimator->work(txn, workingSet, collection, &estimatedDistance);

    if (state == IS_EOF) {
        // We find a document in 4 neighbors at current level, but didn't at previous level.
        //
        // Assuming cell size at current level is d and data is even distributed, the distance
        // between two nearest points are at least d. The following circle with radius of 3 * d
        // covers PI * 9 * d^2, giving at most 30 documents.
        //
        // At the coarsest level, the search area is the whole earth.
        _boundsIncrement = 3 * estimatedDistance;
        invariant(_boundsIncrement > 0.0);

        // Clean up
        _densityEstimator.reset(NULL);
    }

    return state;
}

void GeoNear2DSphereStage::finishSaveState() {
    if (_densityEstimator) {
        _densityEstimator->saveState();
    }
}

void GeoNear2DSphereStage::finishRestoreState(OperationContext* txn) {
    if (_densityEstimator) {
        _densityEstimator->restoreState(txn);
    }
}

void GeoNear2DSphereStage::finishInvalidate(OperationContext* txn,
                                            const RecordId& dl,
                                            InvalidationType type) {
    if (_densityEstimator) {
        _densityEstimator->invalidate(txn, dl, type);
    }
}

StatusWith<NearStage::CoveredInterval*>  //
    GeoNear2DSphereStage::nextInterval(OperationContext* txn,
                                       WorkingSet* workingSet,
                                       Collection* collection) {
    // The search is finished if we searched at least once and all the way to the edge
    if (_currBounds.getInner() >= 0 && _currBounds.getOuter() == _fullBounds.getOuter()) {
        return StatusWith<CoveredInterval*>(NULL);
    }

    //
    // Setup the next interval
    //

    const NearStats* stats = getNearStats();

    if (!stats->intervalStats.empty()) {
        const IntervalStats& lastIntervalStats = stats->intervalStats.back();

        // TODO: Generally we want small numbers of results fast, then larger numbers later
        if (lastIntervalStats.numResultsBuffered < 300)
            _boundsIncrement *= 2;
        else if (lastIntervalStats.numResultsBuffered > 600)
            _boundsIncrement /= 2;
    }

    invariant(_boundsIncrement > 0.0);

    R2Annulus nextBounds(_currBounds.center(),
                         _currBounds.getOuter(),
                         min(_currBounds.getOuter() + _boundsIncrement, _fullBounds.getOuter()));

    bool isLastInterval = (nextBounds.getOuter() == _fullBounds.getOuter());
    _currBounds = nextBounds;

    //
    // Setup the covering region and stages for this interval
    //

    IndexScanParams scanParams;
    scanParams.descriptor = _s2Index;
    scanParams.direction = 1;
    // We use a filter on the key.  The filter rejects keys that don't intersect with the
    // annulus.  An object that is in the annulus might have a key that's not in it and a key
    // that's in it.  As such we can't just look at one key per object.
    //
    // This does force us to do our own deduping of results, though.
    scanParams.doNotDedup = true;
    scanParams.bounds = _nearParams.baseBounds;

    // Because the planner doesn't yet set up 2D index bounds, do it ourselves here
    const string s2Field = _nearParams.nearQuery->field;
    const int s2FieldPosition = getFieldPosition(_s2Index, s2Field);
    scanParams.bounds.fields[s2FieldPosition].intervals.clear();
    OrderedIntervalList* coveredIntervals = &scanParams.bounds.fields[s2FieldPosition];

    TwoDSphereKeyInRegionExpression* keyMatcher =
        new TwoDSphereKeyInRegionExpression(_currBounds, s2Field);

    ExpressionMapping::cover2dsphere(
        keyMatcher->getRegion(), _s2Index->infoObj(), coveredIntervals);

    // IndexScan owns the hash matcher
    IndexScan* scan = new IndexScanWithMatch(txn, scanParams, workingSet, keyMatcher);

    // FetchStage owns index scan
    FetchStage* fetcher(new FetchStage(txn, workingSet, scan, _nearParams.filter, collection));

    return StatusWith<CoveredInterval*>(new CoveredInterval(
        fetcher, true, nextBounds.getInner(), nextBounds.getOuter(), isLastInterval));
}

StatusWith<double> GeoNear2DSphereStage::computeDistance(WorkingSetMember* member) {
    return computeGeoNearDistance(_nearParams, member);
}

}  // namespace mongo
