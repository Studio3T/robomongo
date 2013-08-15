#include "gtest/gtest.h"
#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjbuilder.h>
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/shell/db/ptimeutil.h"

using namespace Robomongo;

TEST(JsonString, BSONObjConversion) 
{
    mongo::BSONObj obj;
    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding,Utc);
    EXPECT_EQ("{}", str);
}

TEST(JsonString, DateConversion)
{
    mongo::BSONObjBuilder toCheck;
    toCheck.append("Date",mongo::Date_t(0));
    mongo::BSONObj obj = toCheck.obj();
    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding,LocalTime);
    EXPECT_EQ("{\n    \"Date\" : ISODate(\"1970-01-01T00:00:00.000Z\")\n}", str);
}

TEST(DateTests, DateConversionEpoch)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(0);

    std::string zeroPt = miutil::isotimeString(time,true,true);
    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString(zeroPt);
    EXPECT_EQ(pt,time);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true))- boost::posix_time::hours(3);
    std::string utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1969-12-31T21:00:00.000Z",utcPtI);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true))+ boost::posix_time::hours(3);
    utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1970-01-01T03:00:00.000Z",utcPtI);
}

TEST(DateTests, DateConversionOne)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(-2177452800000);

    std::string zeroPt = miutil::isotimeString(time,true,true);
    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString(zeroPt);
    EXPECT_EQ(pt,time);

    std::string isoT = miutil::isotimeString(time,true,true);
    pt = miutil::ptimeFromIsoString(isoT);
    std::string utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1901-01-01T00:00:00.000Z",utcPtI);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true,true));
    utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1901-01-01T00:00:00.000Z",utcPtI);
}

TEST(DateTests, DateMax)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(9223372036854775807);

    std::string maxPt = miutil::isotimeString(time,true,true);
    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString(maxPt);
    EXPECT_EQ(pt,time);
}

TEST(DateTests, DateMin)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    mongo::Date_t d(-9223372036854775808);
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(d.millis);

    std::string minPt = miutil::isotimeString(time,true,true);
    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString(minPt);
    EXPECT_EQ(pt,time);
}

TEST(DateTests, DateZero)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(-2208988800000);

    std::string zeroPt = miutil::isotimeString(time,true,true);
    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString(zeroPt);
    EXPECT_EQ(pt,time);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true,true));
    std::string utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1900-01-01T00:00:00.000Z",utcPtI);

    pt = miutil::ptimeFromIsoString(miutil::isotimeString(time,true));
    utcPtI = miutil::isotimeString(pt,true);
    EXPECT_EQ("1900-01-01T00:00:00.000Z",utcPtI);
}

TEST(DateTests, DateMilliseconds)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(978307200127);

    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString("2001-01-01T00:00:00.127Z");
    EXPECT_EQ(pt,time);
}

TEST(DateTests, DateTimezone)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime time = epoch + boost::posix_time::millisec(1376347174411);

    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString("2013-08-13T01:39:34.411+03:00");
    EXPECT_EQ(pt,time);

    boost::posix_time::ptime timeNeg = epoch + boost::posix_time::millisec(1376407774411);
    boost::posix_time::ptime  ptNeg = miutil::ptimeFromIsoString("2013-08-13T01:39:34.411-13:50");
    EXPECT_EQ(ptNeg,timeNeg);
}

TEST(DateTests, DateRandomTest)
{
    boost::posix_time::ptime  pt = miutil::ptimeFromIsoString("2013-08-12T22:39:34.411Z");
    boost::posix_time::ptime  pt2 = miutil::ptimeFromIsoString("2013-08-13T01:39:34.411+03:00");
    EXPECT_EQ(pt,pt2);

    boost::posix_time::ptime  pt3 = miutil::ptimeFromIsoString("2013-08-13T09:09:34.001Z");
    boost::posix_time::ptime  pt4 = miutil::ptimeFromIsoString("2013-08-13T01:39:34.001-07:30");
    EXPECT_EQ(pt3,pt4);
}