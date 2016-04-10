#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/utils/BsonUtils.h"
#include <mongo/client/dbclientinterface.h>

namespace Robomongo
{
    MongoFunction::MongoFunction(const mongo::BSONObj &obj)
    {
        _name = BsonUtils::getField<mongo::String>(obj, "_id");
        _code = obj.getField("value")._asCode();
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
