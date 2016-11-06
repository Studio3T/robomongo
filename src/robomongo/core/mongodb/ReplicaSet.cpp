
#include "robomongo/core/mongodb/ReplicaSet.h"

namespace Robomongo
{
    ReplicaSet::ReplicaSet(const std::string& setName, const mongo::HostAndPort primary,
                           const std::vector<std::pair<std::string, bool>> membersAndHealths, 
                           const std::string errorStr)
                           : setName(setName), primary(primary), membersAndHealths(membersAndHealths), 
                           errorStr(errorStr)
    {}
}
