#include "robomongo/core/domain/MongoUser.h"

#include <mongo/client/dbclientinterface.h>

#include "robomongo/core/utils/BsonUtils.h"

namespace Robomongo
{
    const float MongoUser::minimumSupportedVersion = 2.4f;

    MongoUser::MongoUser(const float version, const mongo::BSONObj &obj) :
        _version(version), _readOnly(false)
    {
        // _id = obj.getField("_id"); // todo
        _name = BsonUtils::getField<mongo::String>(obj, "user");
    }

    MongoUser::MongoUser(const float version) :
        _version(version), _readOnly(false), _role() {}
}
