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

#include "mongo/platform/basic.h"

#include "mongo/db/exec/near.h"

#include "mongo/db/exec/scoped_timer.h"
#include "mongo/db/exec/working_set_common.h"
#include "mongo/util/assert_util.h"

namespace mongo {

using std::vector;

NearStage::NearStage(OperationContext* txn,
                     WorkingSet* workingSet,
                     Collection* collection,
                     PlanStageStats* stats)
    : _txn(txn),
      _workingSet(workingSet),
      _collection(collection),
      _searchState(SearchState_Initializing),
      _stats(stats),
      _nextInterval(NULL) {
    // Ensure we have specific distance search stats unless a child class specified their
    // own distance stats subclass
    if (!_stats->specific) {
        _stats->specific.reset(new NearStats);
    }
}

NearStage::~NearStage() {}

NearStage::CoveredInterval::CoveredInterval(PlanStage* covering,
                                            bool dedupCovering,
                                            double minDistance,
                                            double maxDistance,
                                            bool inclusiveMax)
    : covering(covering),
      dedupCovering(dedupCovering),
      minDistance(minDistance),
      maxDistance(maxDistance),
      inclusiveMax(inclusiveMax) {}


PlanStage::StageState NearStage::initNext() {
    PlanStage::StageState state = initialize(_txn, _workingSet, _collection);
    if (state == PlanStage::IS_EOF) {
        _searchState = SearchState_Buffering;
        return PlanStage::NEED_TIME;
    }

    invariant(state != PlanStage::ADVANCED && state != PlanStage::NEED_FETCH);

    // Propagate NEED_TIME or errors upward.
    return state;
}

PlanStage::StageState NearStage::initialize(OperationContext* txn,
                                            WorkingSet* workingSet,
                                            Collection* collection) {
    return PlanStage::IS_EOF;
}

PlanStage::StageState NearStage::work(WorkingSetID* out) {
    ++_stats->common.works;

    // Adds the amount of time taken by work() to executionTimeMillis.
    ScopedTimer timer(&_stats->common.executionTimeMillis);

    WorkingSetID toReturn = WorkingSet::INVALID_ID;
    Status error = Status::OK();
    PlanStage::StageState nextState = PlanStage::NEED_TIME;

    //
    // Work the search
    //

    if (SearchState_Initializing == _searchState) {
        nextState = initNext();
    } else if (SearchState_Buffering == _searchState) {
        nextState = bufferNext(&toReturn, &error);
    } else if (SearchState_Advancing == _searchState) {
        nextState = advanceNext(&toReturn);
    } else {
        invariant(SearchState_Finished == _searchState);
        nextState = PlanStage::IS_EOF;
    }

    //
    // Handle the results
    //

    if (PlanStage::FAILURE == nextState) {
        *out = WorkingSetCommon::allocateStatusMember(_workingSet, error);
    } else if (PlanStage::ADVANCED == nextState) {
        *out = toReturn;
        ++_stats->common.advanced;
    } else if (PlanStage::NEED_FETCH == nextState) {
        *out = toReturn;
        ++_stats->common.needFetch;
    } else if (PlanStage::NEED_TIME == nextState) {
        ++_stats->common.needTime;
    } else if (PlanStage::IS_EOF == nextState) {
        _stats->common.isEOF = true;
    }

    return nextState;
}

/**
 * Holds a generic search result with a distance computed in some fashion.
 */
struct NearStage::SearchResult {
    SearchResult(WorkingSetID resultID, double distance) : resultID(resultID), distance(distance) {}

    bool operator<(const SearchResult& other) const {
        // We want increasing distance, not decreasing, so we reverse the <
        return distance > other.distance;
    }

    WorkingSetID resultID;
    double distance;
};

// Set "toReturn" when NEED_FETCH.
PlanStage::StageState NearStage::bufferNext(WorkingSetID* toReturn, Status* error) {
    //
    // Try to retrieve the next covered member
    //

    if (!_nextInterval) {
        StatusWith<CoveredInterval*> intervalStatus = nextInterval(_txn, _workingSet, _collection);
        if (!intervalStatus.isOK()) {
            _searchState = SearchState_Finished;
            *error = intervalStatus.getStatus();
            return PlanStage::FAILURE;
        }

        if (NULL == intervalStatus.getValue()) {
            _searchState = SearchState_Finished;
            return PlanStage::IS_EOF;
        }

        // CoveredInterval and its child stage are owned by _childrenIntervals
        _childrenIntervals.push_back(intervalStatus.getValue());
        _nextInterval = _childrenIntervals.back();
        _nextIntervalStats.reset(new IntervalStats());
        _nextIntervalStats->minDistanceAllowed = _nextInterval->minDistance;
        _nextIntervalStats->maxDistanceAllowed = _nextInterval->maxDistance;
        _nextIntervalStats->inclusiveMaxDistanceAllowed = _nextInterval->inclusiveMax;
    }

    WorkingSetID nextMemberID;
    PlanStage::StageState intervalState = _nextInterval->covering->work(&nextMemberID);

    if (PlanStage::IS_EOF == intervalState) {
        _nextInterval = NULL;
        _searchState = SearchState_Advancing;
        return PlanStage::NEED_TIME;
    } else if (PlanStage::FAILURE == intervalState) {
        *error = WorkingSetCommon::getMemberStatus(*_workingSet->get(nextMemberID));
        return intervalState;
    } else if (PlanStage::NEED_FETCH == intervalState) {
        *toReturn = nextMemberID;
        return intervalState;
    } else if (PlanStage::ADVANCED != intervalState) {
        return intervalState;
    }

    //
    // Try to buffer the next covered member
    //

    WorkingSetMember* nextMember = _workingSet->get(nextMemberID);

    // The child stage may not dedup so we must dedup them ourselves.
    if (_nextInterval->dedupCovering && nextMember->hasLoc()) {
        if (_nextIntervalSeen.end() != _nextIntervalSeen.find(nextMember->loc)) {
            _workingSet->free(nextMemberID);
            return PlanStage::NEED_TIME;
        }
    }

    ++_nextIntervalStats->numResultsFound;

    StatusWith<double> distanceStatus = computeDistance(nextMember);

    // Store the member's RecordId, if available, for quick invalidation
    if (nextMember->hasLoc()) {
        _nextIntervalSeen.insert(std::make_pair(nextMember->loc, nextMemberID));
    }

    if (!distanceStatus.isOK()) {
        _searchState = SearchState_Finished;
        *error = distanceStatus.getStatus();
        return PlanStage::FAILURE;
    }

    // If the member's distance is in the current distance interval, add it to our buffered
    // results.
    double memberDistance = distanceStatus.getValue();
    bool inInterval = memberDistance >= _nextInterval->minDistance &&
        (_nextInterval->inclusiveMax ? memberDistance <= _nextInterval->maxDistance
                                     : memberDistance < _nextInterval->maxDistance);

    // Update found distance stats
    if (_nextIntervalStats->minDistanceFound < 0 ||
        memberDistance < _nextIntervalStats->minDistanceFound) {
        _nextIntervalStats->minDistanceFound = memberDistance;
    }

    if (_nextIntervalStats->maxDistanceFound < 0 ||
        memberDistance > _nextIntervalStats->maxDistanceFound) {
        _nextIntervalStats->maxDistanceFound = memberDistance;
    }

    if (inInterval) {
        _resultBuffer.push(SearchResult(nextMemberID, memberDistance));

        ++_nextIntervalStats->numResultsBuffered;

        // Update buffered distance stats
        if (_nextIntervalStats->minDistanceBuffered < 0 ||
            memberDistance < _nextIntervalStats->minDistanceBuffered) {
            _nextIntervalStats->minDistanceBuffered = memberDistance;
        }

        if (_nextIntervalStats->maxDistanceBuffered < 0 ||
            memberDistance > _nextIntervalStats->maxDistanceBuffered) {
            _nextIntervalStats->maxDistanceBuffered = memberDistance;
        }
    } else {
        // We won't pass this WSM up, so deallocate it
        if (nextMember->hasLoc()) {
            _nextIntervalSeen.erase(nextMember->loc);
        }
        _workingSet->free(nextMemberID);
    }

    return PlanStage::NEED_TIME;
}

PlanStage::StageState NearStage::advanceNext(WorkingSetID* toReturn) {
    if (_resultBuffer.empty()) {
        getNearStats()->intervalStats.push_back(*_nextIntervalStats);
        _nextIntervalStats.reset();

        // We're done returning the documents buffered for this annulus, so we can
        // clear out our buffered RecordIds.
        _nextIntervalSeen.clear();

        _searchState = SearchState_Buffering;
        return PlanStage::NEED_TIME;
    }

    *toReturn = _resultBuffer.top().resultID;
    _resultBuffer.pop();

    // If we're returning something, take it out of our RecordId -> WSID map so that future
    // calls to invalidate don't cause us to take action for a RecordId we're done with.
    WorkingSetMember* member = _workingSet->get(*toReturn);
    if (member->hasLoc()) {
        _nextIntervalSeen.erase(member->loc);
    }

    return PlanStage::ADVANCED;
}

bool NearStage::isEOF() {
    return SearchState_Finished == _searchState;
}

void NearStage::saveState() {
    _txn = NULL;
    ++_stats->common.yields;
    for (size_t i = 0; i < _childrenIntervals.size(); i++) {
        _childrenIntervals[i]->covering->saveState();
    }

    // Subclass specific saving, e.g. saving the 2d or 2dsphere density estimator.
    finishSaveState();
}

void NearStage::restoreState(OperationContext* opCtx) {
    invariant(_txn == NULL);
    _txn = opCtx;
    ++_stats->common.unyields;
    for (size_t i = 0; i < _childrenIntervals.size(); i++) {
        _childrenIntervals[i]->covering->restoreState(opCtx);
    }

    // Subclass specific restoring, e.g. restoring the 2d or 2dsphere density estimator.
    finishRestoreState(opCtx);
}

void NearStage::invalidate(OperationContext* txn, const RecordId& dl, InvalidationType type) {
    ++_stats->common.invalidates;
    for (size_t i = 0; i < _childrenIntervals.size(); i++) {
        _childrenIntervals[i]->covering->invalidate(txn, dl, type);
    }

    // If a result is in _resultBuffer and has a RecordId it will be in _nextIntervalSeen as
    // well. It's safe to return the result w/o the RecordId, so just fetch the result.
    unordered_map<RecordId, WorkingSetID, RecordId::Hasher>::iterator seenIt =
        _nextIntervalSeen.find(dl);

    if (seenIt != _nextIntervalSeen.end()) {
        WorkingSetMember* member = _workingSet->get(seenIt->second);
        verify(member->hasLoc());
        WorkingSetCommon::fetchAndInvalidateLoc(txn, member, _collection);
        verify(!member->hasLoc());

        // Don't keep it around in the seen map since there's no valid RecordId anymore
        _nextIntervalSeen.erase(seenIt);
    }

    // Subclass specific invalidation, e.g. passing the invalidation to the 2d or 2dsphere
    // density estimator.
    finishInvalidate(txn, dl, type);
}

vector<PlanStage*> NearStage::getChildren() const {
    vector<PlanStage*> children;
    for (size_t i = 0; i < _childrenIntervals.size(); i++) {
        children.push_back(_childrenIntervals[i]->covering.get());
    }
    return children;
}

PlanStageStats* NearStage::getStats() {
    PlanStageStats* statsClone = _stats->clone();
    for (size_t i = 0; i < _childrenIntervals.size(); ++i) {
        statsClone->children.push_back(_childrenIntervals[i]->covering->getStats());
    }
    return statsClone;
}

StageType NearStage::stageType() const {
    return _stats->stageType;
}

const CommonStats* NearStage::getCommonStats() {
    return &_stats->common;
}

const SpecificStats* NearStage::getSpecificStats() {
    return _stats->specific.get();
}

NearStats* NearStage::getNearStats() {
    return static_cast<NearStats*>(_stats->specific.get());
}

}  // namespace mongo
