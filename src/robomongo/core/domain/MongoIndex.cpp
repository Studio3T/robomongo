#include "robomongo/core/domain/MongoIndex.h"
#include "mongo/bson/bsonobjbuilder.h"

namespace Robomongo
{
    bool getIndex(const mongo::BSONObj &ind,std::string &out)
    {
        bool result = false;
        mongo::BSONElement key = ind.getField("key");
        if(!key.isNull())
        {     
            const char *val = key.valuestr();
            out = val+1;
            result = true;
        }
        return result;
    }
    mongo::BSONObj generateIndex(const std::string &indName)
    {
        mongo::BSONObjBuilder builder;
        builder.append(indName, "1");
        return builder.obj();
    }
}

