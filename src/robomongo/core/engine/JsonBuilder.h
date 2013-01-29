#pragma once

#include <iostream>
#include <string>
#include <mongo/client/dbclient.h>

namespace Robomongo
{
    class JsonBuilder
    {
    public:
        JsonBuilder();
        std::string jsonString(mongo::BSONObj &obj, mongo::JsonStringFormat format, int pretty) const;
        std::string jsonString(mongo::BSONElement &elem, mongo::JsonStringFormat format, bool includeFieldNames, int pretty) const;

    };
}

