#include "gtest/gtest.h"

#include "robomongo/core/domain/App.h"

using namespace Robomongo;

namespace
{
    void assertQuery(const QString &exprectedQuery, const std::string &collectionName, const QString &postfix)
    {
        QString result = detail::buildCollectionQuery(collectionName, postfix);
        ASSERT_EQ(exprectedQuery, result);
    }
}

TEST(BuildName, ValidName)
{
    assertQuery("db.test.find()", "test", "find()");
    assertQuery("db.my_coll.find()", "my_coll", "find()");
    assertQuery("db.MyColl.find()", "MyColl", "find()");
    assertQuery("db.one.find()", "one", "find()");
}

TEST(BuildName, StartsFromDigit)
{
    assertQuery("db['123'].find()", "123", "find()");
    assertQuery("db['1name'].find()", "1name", "find()");
}

TEST(BuildName, InvalidChars)
{
    assertQuery("db['hello-world'].find()", "hello-world", "find()");
    assertQuery("db['%$#@'].find()", "%$#@", "find()");
    assertQuery("db['space inside'].find()", "space inside", "find()");
}

TEST(BuildName, SlashInside)
{
    assertQuery("db['slash\\\\inside'].find()", "slash\\inside", "find()");
}

TEST(BuildName, StartsFromUnderscore)
{
    assertQuery("db.getCollection('_thename').find()", "_thename", "find()");
    assertQuery("db.getCollection('_my_coll').find()", "_my_coll", "find()");
    assertQuery("db.getCollection('___my_coll').find()", "___my_coll", "find()");
}

TEST(BuildName, ReservedName)
{
    assertQuery("db.getCollection('version').find()", "version", "find()");
    assertQuery("db.getCollection('isMaster').find()", "isMaster", "find()");
    assertQuery("db.getCollection('copyDatabase').find()", "copyDatabase", "find()");
    assertQuery("db.getCollection('printReplicationInfo').find()", "printReplicationInfo", "find()");
    assertQuery("db.getCollection('getName').find()", "getName", "find()");
    assertQuery("db.getCollection('prototype').find()", "prototype", "find()");
}
