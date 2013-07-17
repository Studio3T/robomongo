#pragma once
#include <string>
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    bool getIndex(const mongo::BSONObj &ind,std::string &out);
    bool getJsonIndexAndRename(const mongo::BSONObj &ind,mongo::BSONObj &out,const std::string &oldName,const std::string &newName);
}
