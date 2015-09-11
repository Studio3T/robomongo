/**
 *    Copyright (C) 2013 10gen Inc.
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

#include <string>

#include "mongo/base/disallow_copying.h"

namespace mongo {
namespace unittest {
/**
 * An RAII temporary directory that deletes itself and all contents files on scope exit.
 */
class TempDir {
    MONGO_DISALLOW_COPYING(TempDir);

public:
    /**
     * Creates a new unique temporary directory.
     *
     * Throws if this fails for any reason, such as bad permissions.
     *
     * The leaf of the directory path will start with namePrefix and have
     * unspecified characters added to ensure uniqueness.
     *
     * namePrefix must not contain either / or \
     */
    explicit TempDir(const std::string& namePrefix);

    /**
     * Delete the directory and all contents.
     *
     * This only does best-effort. In particular no new files should be created in the directory
     * once the TempDir goes out of scope. Any errors are logged and ignored.
     */
    ~TempDir();

    const std::string& path() {
        return _path;
    }

private:
    std::string _path;
};
}  // namespace unittest
}  // namespace mongo
