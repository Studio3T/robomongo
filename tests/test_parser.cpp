#include "gtest/gtest.h"
#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjbuilder.h>
#include "robomongo/core/utils/BsonUtils.h"
using namespace Robomongo;

TEST(JsonString, BSONObjConversion) 
{
    mongo::BSONObj obj;
    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding);
    EXPECT_EQ("{}", str);
}

TEST(JsonString,DateConversion)
{
    mongo::BSONObjBuilder toCheck;
    toCheck.append("Date",mongo::Date_t(0));
    mongo::BSONObj obj = toCheck.obj();
    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding);
    EXPECT_EQ("{\n    \"Date\" : ISODate(\"1970-01-01T00:00:00.000Z\")\n}", str);
    
    mongo::BSONObjBuilder toCheck3;
    toCheck3.append("Date",mongo::Date_t(-2208988800000));
    obj = toCheck3.obj();
    str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding);
    EXPECT_EQ("{\n    \"Date\" : ISODate(\"?\")\n}", str);
}
