#pragma once
#include <string>
namespace mongo
{
	class BSONObj;
}
namespace Robomongo
{
	bool getIndex(const mongo::BSONObj &ind,std::string &out);
}
