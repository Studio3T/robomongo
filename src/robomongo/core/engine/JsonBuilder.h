#pragma once
#include <iostream>
#include <string>
#include <mongo/client/dbclientinterface.h>

#include "robomongo/core/domain/Enums.h"

namespace Robomongo
{
    namespace JsonBuilder
    {
        std::string jsonString(mongo::BSONObj &obj, mongo::JsonStringFormat format, int pretty, UUIDEncoding uuidEncoding);
        std::string jsonString(mongo::BSONElement &elem, mongo::JsonStringFormat format, bool includeFieldNames, int pretty, UUIDEncoding);
    }
}

