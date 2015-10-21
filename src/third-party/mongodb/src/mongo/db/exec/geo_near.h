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

#pragma once

#include <boost/scoped_ptr.hpp>

#include "mongo/db/exec/near.h"
#include "mongo/db/exec/working_set.h"
#include "mongo/db/exec/plan_stats.h"
#include "mongo/db/index/index_descriptor.h"
#include "mongo/db/geo/geometry_container.h"
#include "mongo/db/matcher/expression.h"
#include "mongo/db/matcher/expression_geo.h"
#include "mongo/db/query/index_bounds.h"

namespace mongo {

/**
 * Generic parameters for a GeoNear search
 */
struct GeoNearParams {
    GeoNearParams() : filter(NULL), nearQuery(NULL), addPointMeta(false), addDistMeta(false) {}

    // MatchExpression to apply to the index keys and fetched documents
    // Not owned here, owned by solution nodes
    MatchExpression* filter;
    // Index scan bounds, not including the geo bounds
    IndexBounds baseBounds;

    // Not owned here
    const GeoNearExpression* nearQuery;
    bool addPointMeta;
    bool addDistMeta;
};

/**
 * Implementation of GeoNear on top of a 2D index
 */
class GeoNear2DStage : public NearStage {
public:
    GeoNear2DStage(const GeoNearParams& nearParams,
                   OperationContext* txn,
                   WorkingSet* workingSet,
                   Collection* collection,
                   IndexDescriptor* twoDIndex);

    virtual ~GeoNear2DStage();

protected:
    virtual StatusWith<CoveredInterval*> nextInterval(OperationContext* txn,
                                                      WorkingSet* workingSet,
                                                      Collection* collection);

    virtual StatusWith<double> computeDistance(WorkingSetMember* member);

    virtual PlanStage::StageState initialize(OperationContext* txn,
                                             WorkingSet* workingSet,
                                             Collection* collection);

private:
    virtual void finishSaveState();

    virtual void finishRestoreState(OperationContext* txn);

    virtual void finishInvalidate(OperationContext* txn, const RecordId& dl, InvalidationType type);

    const GeoNearParams _nearParams;

    // The 2D index we're searching over
    // Not owned here
    IndexDescriptor* const _twoDIndex;

    // The total search annulus
    const R2Annulus _fullBounds;

    // The current search annulus
    R2Annulus _currBounds;

    // Amount to increment the next bounds by
    double _boundsIncrement;

    class DensityEstimator;
    boost::scoped_ptr<DensityEstimator> _densityEstimator;
};

/**
 * Implementation of GeoNear on top of a 2DSphere (S2) index
 */
class GeoNear2DSphereStage : public NearStage {
public:
    GeoNear2DSphereStage(const GeoNearParams& nearParams,
                         OperationContext* txn,
                         WorkingSet* workingSet,
                         Collection* collection,
                         IndexDescriptor* s2Index);

    virtual ~GeoNear2DSphereStage();

protected:
    virtual StatusWith<CoveredInterval*> nextInterval(OperationContext* txn,
                                                      WorkingSet* workingSet,
                                                      Collection* collection);

    virtual StatusWith<double> computeDistance(WorkingSetMember* member);

    virtual PlanStage::StageState initialize(OperationContext* txn,
                                             WorkingSet* workingSet,
                                             Collection* collection);

private:
    virtual void finishSaveState();

    virtual void finishRestoreState(OperationContext* txn);

    virtual void finishInvalidate(OperationContext* txn, const RecordId& dl, InvalidationType type);

    const GeoNearParams _nearParams;

    // The 2D index we're searching over
    // Not owned here
    IndexDescriptor* const _s2Index;

    // The total search annulus
    const R2Annulus _fullBounds;

    // The current search annulus
    R2Annulus _currBounds;

    // Amount to increment the next bounds by
    double _boundsIncrement;

    class DensityEstimator;
    boost::scoped_ptr<DensityEstimator> _densityEstimator;
};

}  // namespace mongo
