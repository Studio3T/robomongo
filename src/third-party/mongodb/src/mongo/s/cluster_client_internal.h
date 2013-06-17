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

/**
 * Useful utilities for clients working on a cluster.  Safe wrapping of operations useful in
 * general for clients using cluster metadata.
 *
 * TODO: See if this stuff is more generally useful, distribute if so.
 */

#pragma once

#include "mongo/base/owned_pointer_map.h"
#include "mongo/base/owned_pointer_vector.h"
#include "mongo/client/dbclientinterface.h"
#include "mongo/s/type_chunk.h"
#include "mongo/s/type_collection.h"

namespace mongo {

    //
    // Helper methods for querying information about a cluster
    //

    /**
     * Tries to check the versions of all active hosts in a cluster.  Not 100% accurate, but pretty
     * effective if hosts are reachable.
     *
     * Returns OK if hosts are compatible as far as we know, RemoteValidationError if hosts are not
     * compatible, and an error Status if anything else goes wrong.
     */
    Status checkClusterMongoVersions(const ConnectionString& configLoc,
                                     const string& minMongoVersion);

    /**
     * Returns all collections in the cluster, found at this moment.
     *
     * Returns OK if loaded successfully, error Status if not.
     */
    Status findAllCollections(const ConnectionString& configLoc,
                              OwnedPointerMap<string, CollectionType>* collections);

    /**
     * Returns all collections in the cluster, but does not throw an error if epochs are not
     * set for the collections.
     *
     * Returns OK if loaded successfully, error Status if not.
     */
    Status findAllCollectionsV3(const ConnectionString& configLoc,
                                OwnedPointerMap<string, CollectionType>* collections);

    /**
     * Returns all chunks for a collection in the cluster.
     *
     * Returns OK if loaded successfully, error Status if not.
     */
    Status findAllChunks(const ConnectionString& configLoc,
                         const string& ns,
                         OwnedPointerVector<ChunkType>* chunks);

    /**
     * Logs to the config.changelog collection
     *
     * Returns OK if loaded successfully, error Status if not.
     */
    Status logConfigChange(const ConnectionString& configLoc,
                           const string& clientHost,
                           const string& ns,
                           const string& description,
                           const BSONObj& details);

    //
    // Needed to normalize exception behavior of connections and cursors
    // TODO: Remove when we refactor the client connection interface to something more consistent.
    //

    // Helper function which throws on bad GLEs for non-SCC config servers
    void _checkGLE(ScopedDbConnection& conn);

    // Helper function which throws for invalid cursor initialization
    DBClientCursor* _safeCursor(auto_ptr<DBClientCursor> cursor);

}
