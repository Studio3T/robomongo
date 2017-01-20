
#include "robomongo/core/mongodb/ReplicaSet.h"

namespace Robomongo
{
    ReplicaSet::ReplicaSet(std::string const& setName, const mongo::HostAndPort primary,
                           std::vector<std::pair<std::string, bool>> const& membersAndHealths, 
                           std::string const& errorStr)
                           : setName(setName), primary(primary), membersAndHealths(membersAndHealths), 
                           errorStr(errorStr)
    {}
}
