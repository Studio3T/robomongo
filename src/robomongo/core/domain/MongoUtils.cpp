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

QString MongoUtils::buildNiceSizeString(int sizeBytes)
{
    return buildNiceSizeString((double) sizeBytes);
}

QString MongoUtils::buildNiceSizeString(double sizeBytes)
{
    if (sizeBytes < 1024 * 100) {
        double kb = ((double) sizeBytes) / 1024;
        return QString("%1 kb").arg(kb, 2, 'f', 2);
    }

    double mb = ((double) sizeBytes) / 1024 / 1024;
    return QString("%1 mb").arg(mb, 2, 'f', 2);
}

/*
void print_ptime_in_ms_from_epoch(const boost::posix_time::ptime& pt)
{
    using boost::posix_time::ptime;
    using namespace boost::gregorian;
    std::cout << (pt-ptime(date(1941, Dec, 7))).total_milliseconds() << "\n";
}*/
