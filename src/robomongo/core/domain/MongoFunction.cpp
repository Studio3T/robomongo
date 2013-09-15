#include "robomongo/core/domain/MongoFunction.h"

#include <mongo/client/dbclient.h>

namespace Robomongo
{
    MongoFunction::MongoFunction(const mongo::BSONObj &obj)
    {
        _name = obj.getField("_id").String();
        _code = obj.getField("value")._asCode().data();
    }

    mongo::BSONObj MongoFunction::toBson() const
    {
        mongo::BSONObjBuilder builder;

        mongo::BSONCode code = mongo::BSONCode(_code);
        builder.append("_id", _name);
        builder.append("value", code);
        mongo::BSONObj obj = builder.obj();
        return obj;
    }
}
