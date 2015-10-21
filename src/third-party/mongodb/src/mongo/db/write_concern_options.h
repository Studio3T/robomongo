/*    Copyright (C) 2014 MongoDB Inc.
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

#include <string>

#include "mongo/base/status.h"
#include "mongo/db/jsobj.h"

namespace mongo {

struct WriteConcernOptions {
public:
    enum SyncMode { NONE, FSYNC, JOURNAL };

    static const int kNoTimeout = 0;
    static const int kNoWaiting = -1;

    static const BSONObj Default;
    static const BSONObj Acknowledged;
    static const BSONObj AllConfigs;
    static const BSONObj Unacknowledged;

    WriteConcernOptions() {
        reset();
    }

    WriteConcernOptions(int numNodes, SyncMode sync, int timeout);

    WriteConcernOptions(const std::string& mode, SyncMode sync, int timeout);

    Status parse(const BSONObj& obj);

    /**
     * Extracts the write concern settings from the BSONObj. The BSON object should have
     * the format:
     *
     * {
     *     ...
     *     secondaryThrottle: <bool>, // optional
     *     _secondaryThrottle: <bool>, // optional
     *     writeConcern: <BSONObj> // optional
     * }
     *
     * Note: secondaryThrottle takes precedence over _secondaryThrottle.
     *
     * Also sets output parameter rawWriteConcernObj if the writeCocnern field exists.
     *
     * Returns OK if the parse was successful. Also returns ErrorCodes::WriteConcernNotDefined
     * when secondary throttle is true but write concern was not specified.
     */
    Status parseSecondaryThrottle(const BSONObj& doc, BSONObj* rawWriteConcernObj);

    /**
     * Return true if the server needs to wait for other secondary nodes to satisfy this
     * write concern setting. Errs on the false positive for non-empty wMode.
     */
    bool shouldWaitForOtherNodes() const;

    void reset() {
        syncMode = NONE;
        wNumNodes = 0;
        wMode = "";
        wTimeout = 0;
    }

    // Returns the BSON representation of this object.
    // Warning: does not return the same object passed on the last parse() call.
    BSONObj toBSON() const;

    SyncMode syncMode;

    // The w parameter for this write concern. The wMode represents the string format and
    // takes precedence over the numeric format wNumNodes.
    int wNumNodes;
    std::string wMode;

    // Timeout in milliseconds.
    int wTimeout;
};
}
