#include "robomongo/core/domain/MongoUtils.h"
#include <mongo/db/json.h>
using namespace std;
#include "mongo/util/md5.hpp"

namespace Robomongo
{
    namespace MongoUtils
    {
        QString buildNiceSizeString(double sizeBytes)
        {
            if (sizeBytes < 1024 * 100) {
                double kb = ((double) sizeBytes) / 1024;
                return QString("%1 kb").arg(kb, 2, 'f', 2);
            }

            double mb = ((double) sizeBytes) / 1024 / 1024;
            return QString("%1 mb").arg(mb, 2, 'f', 2);
        }

        QString buildPasswordHash(const QString &username, const QString &password)
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
    }
}
