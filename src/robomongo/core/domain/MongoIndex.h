#pragma once
#include <string>
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    bool getIndex(const mongo::BSONObj &ind,std::string &out);
}
