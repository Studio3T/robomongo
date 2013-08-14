#include "gtest/gtest.h"
#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjbuilder.h>
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/shell/db/ptimeutil.h"

using namespace Robomongo;

TEST(JsonString, BSONObjConversion) 
{
    mongo::BSONObj obj;
    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding);
    EXPECT_EQ("{}", str);
}

TEST(JsonString, DateConversion)
{
    mongo::BSONObjBuilder toCheck;
    toCheck.append("Date",mongo::Date_t(0));
    mongo::BSONObj obj = toCheck.obj();
    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding);
    EXPECT_EQ("{\n    \"Date\" : ISODate(\"1970-01-01T00:00:00.000Z\")\n}", str);
}

TEST(DateTests, DateConversionEpoch)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(0);

    std::string zeroPt = miutil::isotimeString(time,true,true);
    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString(zeroPt);
    EXPECT_EQ(pt,time);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true)+"+03:00");
    std::string utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1969-12-31T21:00:00.000",utcPtI);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true)+"-03:00");
    utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1970-01-01T03:00:00.000",utcPtI);
}

TEST(DateTests, DateConversionMax)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(-2177452800000);

    std::string zeroPt = miutil::isotimeString(time,true,true);
    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString(zeroPt);
    EXPECT_EQ(pt,time);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true)+"+03:00");
    std::string utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1900-12-31T21:00:00.000",utcPtI);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true)+"-03:00");
    utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1901-01-01T03:00:00.000",utcPtI);
}

TEST(DateTests, DateConversionTimezone)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(1376347174411);

    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString("2013-08-13T01:39:34.411+03:00");
    EXPECT_EQ(pt,time);
}
