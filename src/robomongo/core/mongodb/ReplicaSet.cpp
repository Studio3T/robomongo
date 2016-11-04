#include "ReplicaSet.h"

namespace Robomongo
{
    ReplicaSet::ReplicaSet(const std::string& setName, const mongo::HostAndPort primary,
                           const std::vector<std::pair<std::string, bool>> membersAndHealths)
        : setName(setName), primary(primary), membersAndHealths(membersAndHealths)
    {}
}
