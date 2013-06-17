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
 */

#include "mongo/s/collection_manager.h"

#include "mongo/bson/util/builder.h" // for StringBuilder
#include "mongo/util/mongoutils/str.h"

namespace mongo {

    using mongoutils::str::stream;

    CollectionManager::CollectionManager() { }

    CollectionManager::~CollectionManager() { }

    CollectionManager* CollectionManager::cloneMinus(const ChunkType& chunk,
                                                     const ChunkVersion& newShardVersion,
                                                     string* errMsg) const {
        // The error message string is optional.
        string dummy;
        if (errMsg == NULL) {
            errMsg = &dummy;
        }

        // Check that we have the exact chunk that will be subtracted.
        if (!chunkExists(chunk, errMsg)) {
            // Message was filled in by chunkExists().
            return NULL;
        }

        // If left with no chunks, check that the version is zero.
        if ((_chunksMap.size() == 1) && newShardVersion.isSet()) {
            *errMsg = stream() << "setting version to " << newShardVersion.toString()
                               << " on removing last chunk";
            return NULL;
        }

        // Can't move version backwards when subtracting chunks.  This is what guarantees that
        // no read or write would be taken once we subtract data from the current shard.
        if (newShardVersion <= _maxShardVersion) {
            *errMsg = stream() << "version " << newShardVersion.toString()
                               << " not greater than " << _maxShardVersion.toString();
            return NULL;
        }

        auto_ptr<CollectionManager> manager(new CollectionManager);
        manager->_key = this->_key;
        manager->_key.getOwned();
        manager->_chunksMap = this->_chunksMap;
        manager->_chunksMap.erase(chunk.getMin());
        manager->_maxShardVersion = newShardVersion;
        manager->_maxCollVersion = newShardVersion > _maxCollVersion ?
                                   newShardVersion : this->_maxCollVersion;
        manager->fillRanges();

        dassert(manager->isValid());

        return manager.release();
    }

    static bool overlap(const ChunkType& chunk,
                        const BSONObj& l2,
                        const BSONObj& h2) {
        return ! ((chunk.getMax().woCompare(l2) <= 0) ||
                  (h2.woCompare(chunk.getMin()) <= 0));
    }

    CollectionManager* CollectionManager::clonePlus(const ChunkType& chunk,
                                                    const ChunkVersion& newShardVersion,
                                                    string* errMsg) const {
        // The error message string is optional.
        string dummy;
        if (errMsg == NULL) {
            errMsg = &dummy;
        }

        // It is acceptable to move version backwards (e.g., undoing a migration that went bad
        // during commit) but only cloning away the last chunk may reset the version to 0.
        if (!newShardVersion.isSet()) {
            *errMsg =  "version can't be set to zero";
            return NULL;
        }

        // Check that there isn't any chunk on the interval to be added.
       if (!_chunksMap.empty()) {
           RangeMap::const_iterator it = _chunksMap.lower_bound(chunk.getMax());
            if (it != _chunksMap.begin()) {
                --it;
            }
            if (overlap(chunk, it->first, it->second)) {
                *errMsg = stream() << "ranges overlap, "
                                   << "requested: " << chunk.getMin()
                                   <<" -> " << chunk.getMax() << " "
                                   << "existing: " << it->first.toString()
                                   << " -> " + it->second.toString();
                return NULL;
            }
        }

        auto_ptr<CollectionManager> manager(new CollectionManager);
        manager->_key = this->_key;
        manager->_key.getOwned();
        manager->_chunksMap = this->_chunksMap;
        manager->_chunksMap.insert(make_pair(chunk.getMin().getOwned(), chunk.getMax().getOwned()));
        manager->_maxShardVersion = newShardVersion;
        manager->_maxCollVersion = newShardVersion > _maxCollVersion ?
                                   newShardVersion : this->_maxCollVersion;
        manager->fillRanges();

        dassert(manager->isValid());

        return manager.release();
    }

    static bool contains(const BSONObj& min, const BSONObj& max, const BSONObj& point) {
        return point.woCompare(min) >= 0 && point.woCompare(max) < 0;
    }

    CollectionManager* CollectionManager::cloneSplit(const ChunkType& chunk,
                                                     const vector<BSONObj>& splitKeys,
                                                     const ChunkVersion& newShardVersion,
                                                     string* errMsg) const {
        // The error message string is optional.
        string dummy;
        if (errMsg == NULL) {
            errMsg = &dummy;
        }

        // The version required in both resulting chunks could be simply an increment in the
        // minor portion of the current version.  However, we are enforcing uniqueness over the
        // attributes <ns, version> of the configdb collection 'chunks'.  So in practice, a
        // migrate somewhere may force this split to pick up a version that has the major
        // portion higher than the one that this shard has been using.
        //
        // TODO drop the uniqueness constraint and tighten the check below so that only the
        // minor portion of version changes
        if (newShardVersion <= _maxShardVersion) {
            *errMsg = stream()<< "version " << newShardVersion.toString()
                              << " not greater than " << _maxShardVersion.toString();
            return NULL;
        }

        // Check that we have the exact chunk that will be split and that the split point is
        // valid.
        if (!chunkExists(chunk, errMsg)) {
            return NULL;
        }

        for (vector<BSONObj>::const_iterator it = splitKeys.begin();
             it != splitKeys.end();
             ++it) {
            if (!contains(chunk.getMin(), chunk.getMax(), *it)) {
                *errMsg = stream() << "can split " << chunk.getMin()
                                   << " -> " << chunk.getMax() << " on " << *it;
                return NULL;
            }
        }

        auto_ptr<CollectionManager> manager(new CollectionManager);
        manager->_key = this->_key;
        manager->_key.getOwned();
        manager->_chunksMap = this->_chunksMap;
        manager->_maxShardVersion = newShardVersion; // will increment 2nd, 3rd,... chunks below
        manager->_maxCollVersion = newShardVersion > _maxCollVersion ?
                                   newShardVersion : this->_maxCollVersion;

        BSONObj startKey = chunk.getMin();
        for (vector<BSONObj>::const_iterator it = splitKeys.begin();
             it != splitKeys.end();
             ++it) {
            BSONObj split = *it;
            manager->_chunksMap[chunk.getMin()] = split.getOwned();
            manager->_chunksMap.insert(make_pair(split.getOwned(), chunk.getMax().getOwned()));
            manager->_maxShardVersion.incMinor();
            startKey = split;
        }
        manager->fillRanges();

        dassert(manager->isValid());

        return manager.release();
    }

    bool CollectionManager::belongsToMe(const BSONObj& point) const {
        // For now, collections don't move. So if the collection is not sharded, assume
        // the documet ca be accessed.
        if (_key.isEmpty()) {
            return true;
        }

        dassert(_rangesMap.size() > 0);

        RangeMap::const_iterator it = _rangesMap.upper_bound(point);
        if (it != _rangesMap.begin())
            it--;

        bool good = contains(it->first, it->second, point);

        // Logs if in debugging mode and the point doesn't belong here.
        if(dcompare(!good)) {
            log() << "bad: " << point << " "
                  << it->first << " " << point.woCompare(it->first) << " "
                  << point.woCompare(it->second) << endl;

            for (RangeMap::const_iterator i=_rangesMap.begin(); i!=_rangesMap.end(); ++i) {
                log() << "\t" << i->first << "\t" << i->second << "\t" << endl;
            }
        }

        return good;
    }

    bool CollectionManager::getNextChunk(const BSONObj& lookupKey,
                                         ChunkType* chunk) const {
        if (_chunksMap.empty()) {
            return true;
        }

        RangeMap::const_iterator it;
        if (lookupKey.isEmpty()) {
            it = _chunksMap.begin();
            chunk->setMin(it->first);
            chunk->setMax(it->second);
            return _chunksMap.size() == 1;
        }

        it = _chunksMap.upper_bound(lookupKey);
        if (it != _chunksMap.end()) {
            chunk->setMin(it->first);
            chunk->setMax(it->second);
            return false;
        }

        return true;
    }

    string CollectionManager::toString() const {
        StringBuilder ss;
        ss << " CollectionManager version: " << _maxShardVersion.toString() << " key: " << _key;
        if (_rangesMap.empty()) {
            return ss.str();
        }

        RangeMap::const_iterator it = _rangesMap.begin();
        ss << it->first << " -> " << it->second;
        while (it != _rangesMap.end()) {
            ss << ", "<< it->first << " -> " << it->second;
        }
        return ss.str();
    }

    bool CollectionManager::isValid() const {
        if (_maxShardVersion > _maxCollVersion) {
            return false;
        }

        if (_maxShardVersion.majorVersion() == 0 || _maxCollVersion.majorVersion() == 0)
            return false;

        return true;
    }

    bool CollectionManager::chunkExists(const ChunkType& chunk,
                                        string* errMsg) const {
        RangeMap::const_iterator it = _chunksMap.find(chunk.getMin());
        if (it == _chunksMap.end()) {
            *errMsg =  stream() << "couldn't find chunk " << chunk.getMin()
                                << "->" << chunk.getMax();
            return false;
        }

        if (it->second.woCompare(chunk.getMax()) != 0) {
            *errMsg = stream() << "ranges differ, "
                               << "requested: "  << chunk.getMin()
                               << " -> " << chunk.getMax() << " "
                               << "existing: "
                               << ((it == _chunksMap.end()) ?
                                   "<empty>" :
                                   it->first.toString() + " -> " + it->second.toString());
            return false;
        }

        return true;
    }

    void CollectionManager::fillRanges() {
        if (_chunksMap.empty())
            return;

        // Load the chunk information, coallesceing their ranges.  The version for this shard
        // would be the highest version for any of the chunks.
        RangeMap::const_iterator it = _chunksMap.begin();
        BSONObj min,max;
        while (it != _chunksMap.end()) {
            BSONObj currMin = it->first;
            BSONObj currMax = it->second;
            ++it;

            // coalesce the chunk's bounds in ranges if they are adjacent chunks
            if (min.isEmpty()) {
                min = currMin;
                max = currMax;
                continue;
            }
            if (max == currMin) {
                max = currMax;
                continue;
            }

            _rangesMap.insert(make_pair(min, max));

            min = currMin;
            max = currMax;
        }
        dassert(!min.isEmpty());

        _rangesMap.insert(make_pair(min, max));
    }

} // namespace mongo
