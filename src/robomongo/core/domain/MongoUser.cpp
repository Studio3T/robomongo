#include "robomongo/core/domain/MongoUser.h"

#include <mongo/client/dbclient.h>

namespace Robomongo
{

    MongoUser::MongoUser(const mongo::BSONObj &obj)
    {
        _id = obj.getField("_id").OID();
        _name = obj.getField("user").String();
        _readOnly = obj.getBoolField("readOnly");
        _passwordHash = obj.getStringField("pwd");
    }

    mongo::BSONObj MongoUser::toBson() const
    {
        mongo::BSONObjBuilder builder;

        if (!(_id == mongo::OID())) // if not empty (i.e. not Object("0000000000000000"))
            builder.append("_id", _id);

        builder.append("user", _name);
        builder.append("readOnly", _readOnly);
        builder.append("pwd", _passwordHash);
        mongo::BSONObj obj = builder.obj();
        return obj;
    }
}
