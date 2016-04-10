#include "robomongo/core/domain/MongoUser.h"

#include <mongo/client/dbclientinterface.h>

#include "robomongo/core/utils/BsonUtils.h"

namespace Robomongo
{
    const float MongoUser::minimumSupportedVersion = 2.4f;

    MongoUser::MongoUser(const float version, const mongo::BSONObj &obj) :
        _version(version), _readOnly(false)
    {
        _id = obj.getField("_id").OID();
        _name = BsonUtils::getField<mongo::String>(obj, "user");
        _passwordHash = BsonUtils::getField<mongo::String>(obj, "pwd");
        if (_version<minimumSupportedVersion) {
            _readOnly = BsonUtils::getField<mongo::Bool>(obj, "readOnly");
        }     
        else {
            _userSource = BsonUtils::getField<mongo::String>(obj, "userSource");
            std::vector<mongo::BSONElement> roles = BsonUtils::getField<mongo::Array>(obj, "roles");
            for (std::vector<mongo::BSONElement>::const_iterator it = roles.begin(); it != roles.end(); ++it)
            {
                _role.push_back(BsonUtils::bsonelement_cast<std::string>(*it));
            }
        }
    }

    MongoUser::MongoUser(const float version) :
        _version(version), _readOnly(false), _role() {}

    mongo::BSONObj MongoUser::toBson() const
    {
        mongo::BSONObjBuilder builder;

        if (!(_id == mongo::OID())) // if not empty (i.e. not Object("0000000000000000"))
            builder.append("_id", _id);

        builder.append("user", _name);

        if (!_passwordHash.empty())
            builder.append("pwd", _passwordHash);

        if (_version<minimumSupportedVersion) {
            builder.append("readOnly", _readOnly);
        }
        else {
            if (!_userSource.empty())
                builder.append("userSource", _userSource);

            mongo::BSONArrayBuilder builderA;
            for (RoleType::const_iterator it = _role.begin(); it != _role.end(); ++it)
            {
                builderA << *it;
            }
            mongo::BSONObj arObj = builderA.arr();            
            builder.appendArray("roles", arObj);
        }
        mongo::BSONObj obj = builder.obj();
        return obj;
    }
}
