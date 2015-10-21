/**
 *    Copyright (C) 2014 MongoDB Inc.
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

#include <string>

#include "mongo/db/concurrency/d_concurrency.h"
#include "mongo/db/concurrency/lock_manager_test_help.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

using std::string;

TEST(DConcurrency, GlobalRead) {
    MMAPV1LockerImpl ls;
    Lock::GlobalRead globalRead(&ls);
    ASSERT(ls.isR());
}

TEST(DConcurrency, GlobalWrite) {
    MMAPV1LockerImpl ls;
    Lock::GlobalWrite globalWrite(&ls);
    ASSERT(ls.isW());
}

TEST(DConcurrency, GlobalWriteAndGlobalRead) {
    MMAPV1LockerImpl ls;

    Lock::GlobalWrite globalWrite(&ls);
    ASSERT(ls.isW());

    {
        Lock::GlobalRead globalRead(&ls);
        ASSERT(ls.isW());
    }

    ASSERT(ls.isW());
}

TEST(DConcurrency, GlobalLockS_Timeout) {
    MMAPV1LockerImpl ls;
    Lock::GlobalLock globalWrite(&ls, MODE_X, 0);
    ASSERT(globalWrite.isLocked());

    {
        MMAPV1LockerImpl lsTry;
        Lock::GlobalLock globalReadTry(&lsTry, MODE_S, 1);
        ASSERT(!globalReadTry.isLocked());
    }
}

TEST(DConcurrency, GlobalLockX_Timeout) {
    MMAPV1LockerImpl ls;
    Lock::GlobalLock globalWrite(&ls, MODE_X, 0);
    ASSERT(globalWrite.isLocked());

    {
        MMAPV1LockerImpl lsTry;
        Lock::GlobalLock globalWriteTry(&lsTry, MODE_X, 1);
        ASSERT(!globalWriteTry.isLocked());
    }
}

TEST(DConcurrency, GlobalLockS_NoTimeoutDueToGlobalLockS) {
    MMAPV1LockerImpl ls;
    Lock::GlobalRead globalRead(&ls);

    MMAPV1LockerImpl lsTry;
    Lock::GlobalLock globalReadTry(&lsTry, MODE_S, 1);

    ASSERT(globalReadTry.isLocked());
}

TEST(DConcurrency, GlobalLockX_TimeoutDueToGlobalLockS) {
    MMAPV1LockerImpl ls;
    Lock::GlobalRead globalRead(&ls);

    MMAPV1LockerImpl lsTry;
    Lock::GlobalLock globalWriteTry(&lsTry, MODE_X, 1);

    ASSERT(!globalWriteTry.isLocked());
}

TEST(DConcurrency, GlobalLockS_TimeoutDueToGlobalLockX) {
    MMAPV1LockerImpl ls;
    Lock::GlobalWrite globalWrite(&ls);

    MMAPV1LockerImpl lsTry;
    Lock::GlobalLock globalReadTry(&lsTry, MODE_S, 1);

    ASSERT(!globalReadTry.isLocked());
}

TEST(DConcurrency, GlobalLockX_TimeoutDueToGlobalLockX) {
    MMAPV1LockerImpl ls;
    Lock::GlobalWrite globalWrite(&ls);

    MMAPV1LockerImpl lsTry;
    Lock::GlobalLock globalWriteTry(&lsTry, MODE_X, 1);

    ASSERT(!globalWriteTry.isLocked());
}

TEST(DConcurrency, TempReleaseGlobalWrite) {
    MMAPV1LockerImpl ls;
    Lock::GlobalWrite globalWrite(&ls);

    {
        Lock::TempRelease tempRelease(&ls);
        ASSERT(!ls.isLocked());
    }

    ASSERT(ls.isW());
}

TEST(DConcurrency, TempReleaseRecursive) {
    MMAPV1LockerImpl ls;
    Lock::GlobalWrite globalWrite(&ls);
    Lock::DBLock lk(&ls, "SomeDBName", MODE_X);

    {
        Lock::TempRelease tempRelease(&ls);
        ASSERT(ls.isW());
        ASSERT(ls.isDbLockedForMode("SomeDBName", MODE_X));
    }

    ASSERT(ls.isW());
}

TEST(DConcurrency, DBLockTakesS) {
    MMAPV1LockerImpl ls;

    Lock::DBLock dbRead(&ls, "db", MODE_S);

    const ResourceId resIdDb(RESOURCE_DATABASE, string("db"));
    ASSERT(ls.getLockMode(resIdDb) == MODE_S);
}

TEST(DConcurrency, DBLockTakesX) {
    MMAPV1LockerImpl ls;

    Lock::DBLock dbWrite(&ls, "db", MODE_X);

    const ResourceId resIdDb(RESOURCE_DATABASE, string("db"));
    ASSERT(ls.getLockMode(resIdDb) == MODE_X);
}

TEST(DConcurrency, DBLockTakesISForAdminIS) {
    DefaultLockerImpl ls;

    Lock::DBLock dbRead(&ls, "admin", MODE_IS);

    ASSERT(ls.getLockMode(resourceIdAdminDB) == MODE_IS);
}

TEST(DConcurrency, DBLockTakesSForAdminS) {
    DefaultLockerImpl ls;

    Lock::DBLock dbRead(&ls, "admin", MODE_S);

    ASSERT(ls.getLockMode(resourceIdAdminDB) == MODE_S);
}

TEST(DConcurrency, DBLockTakesXForAdminIX) {
    DefaultLockerImpl ls;

    Lock::DBLock dbWrite(&ls, "admin", MODE_IX);

    ASSERT(ls.getLockMode(resourceIdAdminDB) == MODE_X);
}

TEST(DConcurrency, DBLockTakesXForAdminX) {
    DefaultLockerImpl ls;

    Lock::DBLock dbWrite(&ls, "admin", MODE_X);

    ASSERT(ls.getLockMode(resourceIdAdminDB) == MODE_X);
}

TEST(DConcurrency, MultipleWriteDBLocksOnSameThread) {
    MMAPV1LockerImpl ls;

    Lock::DBLock r1(&ls, "db1", MODE_X);
    Lock::DBLock r2(&ls, "db1", MODE_X);

    ASSERT(ls.isDbLockedForMode("db1", MODE_X));
}

TEST(DConcurrency, MultipleConflictingDBLocksOnSameThread) {
    MMAPV1LockerImpl ls;

    Lock::DBLock r1(&ls, "db1", MODE_X);
    Lock::DBLock r2(&ls, "db1", MODE_S);

    ASSERT(ls.isDbLockedForMode("db1", MODE_X));
    ASSERT(ls.isDbLockedForMode("db1", MODE_S));
}

TEST(DConcurrency, IsDbLockedForSMode) {
    const std::string dbName("db");

    MMAPV1LockerImpl ls;

    Lock::DBLock dbLock(&ls, dbName, MODE_S);

    ASSERT(ls.isDbLockedForMode(dbName, MODE_IS));
    ASSERT(!ls.isDbLockedForMode(dbName, MODE_IX));
    ASSERT(ls.isDbLockedForMode(dbName, MODE_S));
    ASSERT(!ls.isDbLockedForMode(dbName, MODE_X));
}

TEST(DConcurrency, IsDbLockedForXMode) {
    const std::string dbName("db");

    MMAPV1LockerImpl ls;

    Lock::DBLock dbLock(&ls, dbName, MODE_X);

    ASSERT(ls.isDbLockedForMode(dbName, MODE_IS));
    ASSERT(ls.isDbLockedForMode(dbName, MODE_IX));
    ASSERT(ls.isDbLockedForMode(dbName, MODE_S));
    ASSERT(ls.isDbLockedForMode(dbName, MODE_X));
}

TEST(DConcurrency, IsCollectionLocked_DB_Locked_IS) {
    const std::string ns("db1.coll");

    MMAPV1LockerImpl ls;

    Lock::DBLock dbLock(&ls, "db1", MODE_IS);

    {
        Lock::CollectionLock collLock(&ls, ns, MODE_IS);

        ASSERT(ls.isCollectionLockedForMode(ns, MODE_IS));
        ASSERT(!ls.isCollectionLockedForMode(ns, MODE_IX));

        // TODO: This is TRUE because Lock::CollectionLock converts IS lock to S
        ASSERT(ls.isCollectionLockedForMode(ns, MODE_S));

        ASSERT(!ls.isCollectionLockedForMode(ns, MODE_X));
    }

    {
        Lock::CollectionLock collLock(&ls, ns, MODE_S);

        ASSERT(ls.isCollectionLockedForMode(ns, MODE_IS));
        ASSERT(!ls.isCollectionLockedForMode(ns, MODE_IX));
        ASSERT(ls.isCollectionLockedForMode(ns, MODE_S));
        ASSERT(!ls.isCollectionLockedForMode(ns, MODE_X));
    }
}

TEST(DConcurrency, IsCollectionLocked_DB_Locked_IX) {
    const std::string ns("db1.coll");

    MMAPV1LockerImpl ls;

    Lock::DBLock dbLock(&ls, "db1", MODE_IX);

    {
        Lock::CollectionLock collLock(&ls, ns, MODE_IX);

        // TODO: This is TRUE because Lock::CollectionLock converts IX lock to X
        ASSERT(ls.isCollectionLockedForMode(ns, MODE_IS));

        ASSERT(ls.isCollectionLockedForMode(ns, MODE_IX));
        ASSERT(ls.isCollectionLockedForMode(ns, MODE_S));
        ASSERT(ls.isCollectionLockedForMode(ns, MODE_X));
    }

    {
        Lock::CollectionLock collLock(&ls, ns, MODE_X);

        ASSERT(ls.isCollectionLockedForMode(ns, MODE_IS));
        ASSERT(ls.isCollectionLockedForMode(ns, MODE_IX));
        ASSERT(ls.isCollectionLockedForMode(ns, MODE_S));
        ASSERT(ls.isCollectionLockedForMode(ns, MODE_X));
    }
}

}  // namespace mongo
