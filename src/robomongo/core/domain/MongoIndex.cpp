#include "robomongo/core/domain/MongoIndex.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/db/json.h"

namespace Robomongo
{
    bool getIndex(const mongo::BSONObj &ind,std::string &out)
    {
        bool result = false;
        mongo::BSONElement key = ind.getField("key");
        if(!key.isNull())
        {     
            const char *val = key.valuestr();
            out = val+1;
            result = true;
        }
        return result;
    }
    bool getJsonIndexAndRename(const mongo::BSONObj &ind,mongo::BSONObj &out,const std::string &oldName,const std::string &newName)
    {
        bool result = false;
        std::string str = ind.toString();
        size_t start = str.find("key:");
        if(start!=std::string::npos){
            size_t end = str.find("ns:",start);
            if(end!=std::string::npos){
                 std::string indexString = str.substr(start+4,end-(start+4));
                size_t pos = indexString.find(oldName);
                if(pos!=std::string::npos){
                    indexString.replace(pos,oldName.size(),newName);
                    out = mongo::fromjson(indexString);
                    result=true;
                }
            }
        }
        return result;
    }
}

