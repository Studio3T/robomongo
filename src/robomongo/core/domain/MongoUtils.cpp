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

        std::string buildPasswordHash(const std::string &username, const std::string &password)
        {
            std::string sum = username + ":mongo:" + password;
            const char * s = sum.c_str();

            mongo::md5digest d;
            md5_state_t st;
            md5_init(&st);
            md5_append( &st , (const md5_byte_t*)s , strlen( s ) );
            md5_finish(&st, d);

            return mongo::digestToString(d);
        }
    }
}
