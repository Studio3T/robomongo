/**
 *    Copyright 2013 10gen Inc.
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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#include <algorithm>

#include "mongo/client/dbclientinterface.h"
#include "mongo/db/jsobj.h"

namespace mongo {

class QueryMessage;

/**
 * Parses the QueryMessage received from the user and makes the various fields more easily
 * accessible.
 */
class LiteParsedQuery {
public:
    /**
     * Encapsulates various options that can be passed in order to affect the behavior
     * of a query beyond the basic query specification (filter, sort, projection, skip, limit,
     * batchSize).
     */
    struct Options {
        Options();

        /**
         * Fills in the LiteParsedQuery::Options* 'out' by parsing the BSON format,
         * 'optionsObj'.
         */
        static Status parseFromBSON(const BSONObj& optionsObj, Options* out);

        /**
         * Restores all options to their default values.
         */
        void clear();

        /**
         * Several query options are passed as flags within an integer field in the legacy
         * OP_QUERY message. Use this method to initialize this Options struct based on the
         * legacy format.
         */
        void initFromInt(int options);

        /**
         * Converts those options which used to be contained in the integer 'flags' field of
         * the OP_QUERY message to their legacy format.
         *
         * TODO: The bit vector format returned here is historical. All uses of it should
         * be removed.
         */
        int toInt() const;

        //
        // Member fields
        //

        std::string comment;

        int maxScan;
        int maxTimeMS;

        BSONObj min;
        BSONObj max;

        bool returnKey;
        bool showDiskLoc;
        bool snapshot;
        bool hasReadPref;

        // Options that can be specified in the OP_QUERY 'flags' header.
        bool tailable;
        bool slaveOk;
        bool oplogReplay;
        bool noCursorTimeout;
        bool awaitData;
        bool exhaust;
        bool partial;
    };

    /**
     * Parses a find command object, 'cmdObj'. Caller must indicate whether or not
     * this lite parsed query is an explained query or not via 'isExplain'.
     *
     * On success, fills in the out-parameter 'parsedQuery' and returns an OK status.
     * The caller takes ownership of *out.
     *
     * Returns a failure status if 'cmdObj' is not well formed. On failure the caller
     * is not responsible for deleting *out.
     */
    static Status make(const std::string& fullns,
                       const BSONObj& cmdObj,
                       bool isExplain,
                       LiteParsedQuery** out);

    /**
     * Helper functions to parse maxTimeMS from a command object.  Returns the contained value,
     * or an error on parsing fail.  When passed an EOO-type element, returns 0 (special value
     * for "allow to run indefinitely").
     */
    static StatusWith<int> parseMaxTimeMSCommand(const BSONObj& cmdObj);

    /**
     * Same as parseMaxTimeMSCommand, but for a query object.
     */
    static StatusWith<int> parseMaxTimeMSQuery(const BSONObj& queryObj);

    /**
     * Helper function to identify text search sort key
     * Example: {a: {$meta: "textScore"}}
     */
    static bool isTextScoreMeta(BSONElement elt);

    /**
     * Helper function to identify diskLoc projection
     * Example: {a: {$meta: "diskloc"}}.
     */
    static bool isDiskLocMeta(BSONElement elt);

    /**
     * Helper function to validate a sort object.
     * Returns true if each element satisfies one of:
     * 1. a number with value 1
     * 2. a number with value -1
     * 3. isTextScoreMeta
     */
    static bool isValidSortOrder(const BSONObj& sortObj);

    /**
     * Returns true if the query described by "query" should execute
     * at an elevated level of isolation (i.e., $isolated was specified).
     */
    static bool isQueryIsolated(const BSONObj& query);

    // Names of the maxTimeMS command and query option.
    static const std::string cmdOptionMaxTimeMS;
    static const std::string queryOptionMaxTimeMS;

    // Names of the $meta projection values.
    static const std::string metaTextScore;
    static const std::string metaGeoNearDistance;
    static const std::string metaGeoNearPoint;
    static const std::string metaDiskLoc;
    static const std::string metaIndexKey;

    const std::string& ns() const {
        return _ns;
    }
    bool isLocalDB() const {
        return _ns.compare(0, 6, "local.") == 0;
    }

    const BSONObj& getFilter() const {
        return _filter;
    }
    const BSONObj& getProj() const {
        return _proj;
    }
    const BSONObj& getSort() const {
        return _sort;
    }
    const BSONObj& getHint() const {
        return _hint;
    }

    int getSkip() const {
        return _skip;
    }
    int getLimit() const {
        return _limit;
    }
    int getBatchSize() const {
        return _batchSize;
    }
    int getNumToReturn() const {
        return std::min(_limit, _batchSize);
    }
    bool wantMore() const {
        return _wantMore;
    }
    bool hasReadPref() const {
        return _options.hasReadPref;
    }

    bool isExplain() const {
        return _explain;
    }
    bool isSnapshot() const {
        return _options.snapshot;
    }
    bool returnKey() const {
        return _options.returnKey;
    }
    bool showDiskLoc() const {
        return _options.showDiskLoc;
    }

    const BSONObj& getMin() const {
        return _options.min;
    }
    const BSONObj& getMax() const {
        return _options.max;
    }
    int getMaxScan() const {
        return _options.maxScan;
    }
    int getMaxTimeMS() const {
        return _options.maxTimeMS;
    }

    const Options& getOptions() const {
        return _options;
    }

    //
    // Old parsing code: SOON TO BE DEPRECATED.
    //

    /**
     * Parse the provided QueryMessage and set *out to point to the output.
     *
     * Return Status::OK() if parsing succeeded.  Caller owns *out.
     * Otherwise, *out is invalid and the returned Status indicates why parsing failed.
     */
    static Status make(const QueryMessage& qm, LiteParsedQuery** out);

    /**
     * Fills out a LiteParsedQuery.  Used for debugging and testing, when we don't have a
     * QueryMessage.
     */
    static Status make(const std::string& ns,
                       int ntoskip,
                       int ntoreturn,
                       int queryoptions,
                       const BSONObj& query,
                       const BSONObj& proj,
                       const BSONObj& sort,
                       const BSONObj& hint,
                       const BSONObj& minObj,
                       const BSONObj& maxObj,
                       bool snapshot,
                       bool explain,
                       LiteParsedQuery** out);

private:
    LiteParsedQuery();

    /**
     * Parsing code calls this after construction of the LPQ is complete. There are additional
     * semantic properties that must be checked even if "lexically" the parse is OK.
     */
    Status validate() const;

    Status init(const std::string& ns,
                int ntoskip,
                int ntoreturn,
                int queryOptions,
                const BSONObj& queryObj,
                const BSONObj& proj,
                bool fromQueryMessage);

    Status initFullQuery(const BSONObj& top);

    static StatusWith<int> parseMaxTimeMS(const BSONElement& maxTimeMSElt);

    /**
     * Updates the projection object with a $meta projection for the returnKey option.
     */
    void addReturnKeyMetaProj();

    /**
     * Updates the projection object with a $meta projection for the showDiskLoc option.
     */
    void addShowDiskLocMetaProj();

    std::string _ns;
    int _skip;
    int _limit;
    int _batchSize;
    BSONObj _filter;
    BSONObj _sort;
    BSONObj _proj;
    bool _wantMore;
    bool _explain;

    // The hint provided, if any.  If the hint was by index key pattern, the value of '_hint' is
    // the key pattern hinted.  If the hint was by index name, the value of '_hint' is
    // {$hint: <String>}, where <String> is the index name hinted.
    BSONObj _hint;

    Options _options;
};

}  // namespace mongo
