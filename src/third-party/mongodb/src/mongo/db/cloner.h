// cloner.h - copy a database (export/import basically)

/**
 *    Copyright (C) 2011 10gen Inc.
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

#include "mongo/client/dbclientinterface.h"
#include "mongo/base/disallow_copying.h"

namespace mongo {

struct CloneOptions;
class DBClientBase;
class NamespaceString;
class OperationContext;


class Cloner {
    MONGO_DISALLOW_COPYING(Cloner);

public:
    Cloner();

    void setConnection(DBClientBase* c) {
        _conn.reset(c);
    }

    /** copy the entire database */
    bool go(OperationContext* txn,
            const std::string& toDBName,
            const std::string& masterHost,
            const CloneOptions& opts,
            std::set<std::string>* clonedColls,
            std::string& errmsg,
            int* errCode = 0);

    bool copyCollection(OperationContext* txn,
                        const std::string& ns,
                        const BSONObj& query,
                        std::string& errmsg,
                        bool mayYield,
                        bool mayBeInterrupted,
                        bool copyIndexes = true,
                        bool logForRepl = true);

private:
    void copy(OperationContext* txn,
              const std::string& toDBName,
              const NamespaceString& from_ns,
              const BSONObj& from_opts,
              const NamespaceString& to_ns,
              bool logForRepl,
              bool masterSameProcess,
              bool slaveOk,
              bool mayYield,
              bool mayBeInterrupted,
              Query q);

    void copyIndexes(OperationContext* txn,
                     const std::string& toDBName,
                     const NamespaceString& from_ns,
                     const BSONObj& from_opts,
                     const NamespaceString& to_ns,
                     bool logForRepl,
                     bool masterSameProcess,
                     bool slaveOk,
                     bool mayYield,
                     bool mayBeInterrupted);

    struct Fun;
    std::auto_ptr<DBClientBase> _conn;
};

/**
 *  slaveOk     - if true it is ok if the source of the data is !ismaster.
 *  useReplAuth - use the credentials we normally use as a replication slave for the cloning
 *  snapshot    - use $snapshot mode for copying collections.  note this should not be used
 *                when it isn't required, as it will be slower.  for example,
 *                repairDatabase need not use it.
 */
struct CloneOptions {
    CloneOptions() {
        logForRepl = true;
        slaveOk = false;
        useReplAuth = false;
        snapshot = true;
        mayYield = true;
        mayBeInterrupted = false;

        syncData = true;
        syncIndexes = true;
    }

    std::string fromDB;
    std::set<std::string> collsToIgnore;

    bool logForRepl;
    bool slaveOk;
    bool useReplAuth;
    bool snapshot;
    bool mayYield;
    bool mayBeInterrupted;

    bool syncData;
    bool syncIndexes;
};

}  // namespace mongo
