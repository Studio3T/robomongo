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

#pragma once

#include "mongo/base/disallow_copying.h"
#include "mongo/db/jsobj.h"
#include "mongo/s/chunk_version.h"
#include "mongo/s/type_chunk.h"

namespace mongo {

    class MetadataLoader;

    /**
     * The collection manager has metadata information about a collection, in particular the
     * sharding information. It's main goal in life is to be capable of answering if a certain
     * document belongs to it or not. (In some scenarios such as chunk migration, a given
     * document is in a shard but cannot be accessed.)
     *
     * To build a collection from config data, please check the MetadataLoader. The methods
     * here allow building a new incarnation of a collection's metadata based on an existing
     * one (e.g, we're splitting in a given collection.).
     *
     * This class is immutable once constructed.
     */
    class CollectionManager {
        MONGO_DISALLOW_COPYING(CollectionManager);
    public:
        ~CollectionManager();

        //
        // cloning support
        //

        /**
         * Returns a new manager's instance based on 'this's state by removing 'chunk'. The new
         * manager will be at 'newShardVersion', which should be higher than the current
         * one. When cloning away the last chunk, 'newShardVersion' must be zero. In any case,
         * the caller owns the new manager when the cloning is succesful.
         *
         * If a new manager can't be created, returns NULL and fills in 'errMsg', if it was
         * provided.
         */
        CollectionManager* cloneMinus(const ChunkType& chunk,
                                      const ChunkVersion& newShardVersion,
                                      string* errMsg) const;

        /**
         * Returns a new manager's instance based on 'this's state by adding 'chunk'. The new
         * manager will be at 'newShardVersion', which should be higher than the current
         * one. It can never be zero, though (see cloneMinus). The caller owns the new manager.
         *
         * If a new manager can't be created, returns NULL and fills in 'errMsg', if it was
         * provided.
         */
        CollectionManager* clonePlus(const ChunkType& chunk,
                                     const ChunkVersion& newShardVersion,
                                     string* errMsg) const;

        /**
         * Returns a new manager's instance by splitting an existing 'chunk' at the points
         * describe by 'splitKeys'. The first resulting chunk will have 'newShardVersion' and
         * subsequent one would have that with the minor version incremented at each chunk. The
         * caller owns the manager.
         *
         * If a new manager can't be created, returns NULL and fills in 'errMsg', if it was
         * provided.
         */
        CollectionManager* cloneSplit(const ChunkType& chunk,
                                      const vector<BSONObj>& splitKeys,
                                      const ChunkVersion& newShardVersion,
                                      string* errMsg) const;

        //
        // verification logic
        //

        /**
         * Returns true the document 'doc' belongs to this chunkset. Recall that documents of
         * an in-flight chunk migration may be present and should not be considered part of the
         * collection / chunkset yet. 'doc' must contain the sharding key and, optionally,
         * other attributes.
         */
        bool belongsToMe(const BSONObj& doc) const;

        /**
         * Given the chunk's min key (or empty doc) in 'lookupKey', gets the boundaries of the
         * chunk following that one (the first), and fills in 'foundChunk' with those
         * boundaries.  If the next chunk happens to be the last one, returns true otherwise
         * false.
         */
        bool getNextChunk(const BSONObj& lookupKey, ChunkType* foundChunk) const;

        //
        // accessors
        //

        ChunkVersion getMaxCollVersion() const { return _maxCollVersion; }

        ChunkVersion getMaxShardVersion() const { return _maxShardVersion; }

        BSONObj getKey() const { return _key; }

        size_t getNumChunks() const { return _chunksMap.size(); }

        string toString() const;

    private:
        // Effectively, the MetadataLoader is this class's builder. So we open an exception
        // and grant it friendship.
        friend class MetadataLoader;

        // a version for this collection that identifies the collection incarnation (ie, a
        // dropped and recreated collection with the same name would have a different version)
        ChunkVersion _maxCollVersion;

        //
        // sharded state below, for when the colelction gets sharded
        //

        // highest ChunkVersion for which this manager's information is accurate
        ChunkVersion _maxShardVersion;

        // key pattern for chunks under this range
        BSONObj _key;

        // a map from a min key into the chunk's (or range's) max boundary
        typedef map< BSONObj, BSONObj , BSONObjCmp > RangeMap;
        RangeMap _chunksMap;

        // A second map from a min key into a range or contiguous chunks. The map is redundant
        // w.r.t. _chunkMap but we expect high chunk contiguity, especially in small
        // installations.
        RangeMap _rangesMap;

        /**
         * Use the MetadataLoader to build new managers using config server data, or the
         * clone*() methods to use existing managers to build new ones.
         */
        CollectionManager();

        /**
         * Returns true if this manager was loaded with all necessary information.
         */
        bool isValid() const;

        /**
         * Returns true if 'chunk' exist in this * collections's chunkset.
         */
        bool chunkExists(const ChunkType& chunk, string* errMsg) const;

        /**
         * Try to find chunks that are adjacent and record these intervals in the _rangesMap
         */
        void fillRanges();

    };

} // namespace mongo
