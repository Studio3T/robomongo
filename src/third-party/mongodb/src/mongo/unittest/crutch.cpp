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
*    must comply with the GNU Affero General Public License in all respects
*    for all of the code used other than as permitted herein. If you modify
*    file(s) with this exception, you may extend this exception to your
*    version of the file(s), but you are not obligated to do so. If you do not
*    wish to do so, delete this exception statement from your version. If you
*    delete this exception statement from all source files in the program,
*    then also delete it in the license file.
*/

/**
 * This file should go away.  It contains stubs of functions that were needed to link the unit test
 * framework.  As we refactor the system, the contents of this file should _ONLY_ shrink, and
 * eventually it should contain nothing.
 */

#include "mongo/platform/basic.h"

#include <string>

#include "mongo/client/dbclientinterface.h"
#include "mongo/util/exit_code.h"

namespace mongo {

bool inShutdown() {
    return false;
}

class OperationContext;

DBClientBase* createDirectClient(OperationContext* txn) {
    fassertFailed(17249);
    return NULL;
}

bool haveLocalShardingInfo(const std::string& ns) {
    return false;
}

void dbexit(ExitCode rc, const char* why) {
    invariant(!"unittests shouldn't call dbexit");
}

void exitCleanly(ExitCode code) {
    invariant(!"unittests shouldn't call exitCleanly");
}

#ifdef _WIN32
void signalShutdown() {}

namespace ntservice {
bool shouldStartService() {
    return false;
}
}
#endif

}  // namespace mongo
