#include "robomongo/core/domain/MongoUtils.h"

#include "mongo/util/md5.hpp"

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

QString MongoUtils::buildPasswordHash(const QString &username, const QString &password)
{
    QString sum = username + ":mongo:" + password;
    QByteArray bytes = sum.toUtf8();
    const char * s = bytes.constData();

    mongo::md5digest d;
    md5_state_t st;
    md5_init(&st);
    md5_append( &st , (const md5_byte_t*)s , strlen( s ) );
    md5_finish(&st, d);

    std::string hash = mongo::digestToString(d);
    return QString::fromUtf8(hash.c_str(), hash.size());
}

/*
void print_ptime_in_ms_from_epoch(const boost::posix_time::ptime& pt)
{
    using boost::posix_time::ptime;
    using namespace boost::gregorian;
    std::cout << (pt-ptime(date(1941, Dec, 7))).total_milliseconds() << "\n";
}*/
