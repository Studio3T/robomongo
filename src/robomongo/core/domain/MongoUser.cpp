#include "robomongo/core/domain/MongoUser.h"

#include <mongo/client/dbclient.h>
#include "robomongo/core/utils/BsonUtils.h"

namespace
{
    const char *emptyArray = "[]";
}

namespace Robomongo
{

    MongoUser::MongoUser(const mongo::BSONObj &obj)
    {
        _id = obj.getField("_id").OID();
        _name = BsonUtils::getField<mongo::String>(obj,"user");
        _passwordHash = obj.getStringField("pwd");
        std::string str = obj.toString();
        std::vector<mongo::BSONElement> roles = BsonUtils::getField<mongo::Array>(obj,"roles");
        _role =  BsonUtils::bsonArrayToString(roles);
    }

    MongoUser::MongoUser()
        :_role(emptyArray)
    {
    }

    mongo::BSONObj MongoUser::toBson() const
    {
        mongo::BSONObjBuilder builder;

        if (!(_id == mongo::OID())) // if not empty (i.e. not Object("0000000000000000"))
            builder.append("_id", _id);

        builder.append("user", _name);
        builder.append("pwd", _passwordHash);
        mongo::BSONObj arObj = BSON_ARRAY(_role.c_str());
        builder.appendArray("roles", arObj);
        mongo::BSONObj obj = builder.obj();
        
        return obj;
    }
}
