#include "robomongo/core/domain/MongoUtils.h"

using namespace Robomongo;

QString MongoUtils::getName(const QString &fullName)
{
    int dot = fullName.indexOf('.');
    return fullName.mid(dot + 1);
}

mongo::BSONObj MongoUtils::fromjson(const QString &text)
{
    QByteArray utf = text.toUtf8();
    mongo::BSONObj obj = mongo::fromjson(utf.data());
    return obj;
}

/*
void print_ptime_in_ms_from_epoch(const boost::posix_time::ptime& pt)
{
    using boost::posix_time::ptime;
    using namespace boost::gregorian;
    std::cout << (pt-ptime(date(1941, Dec, 7))).total_milliseconds() << "\n";
}*/
