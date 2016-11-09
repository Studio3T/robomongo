#pragma once

#include <mongo/util/net/hostandport.h>

namespace Robomongo
{
    // todo: rename ReplicaSetInfo
    struct ReplicaSet
    {
        ReplicaSet(const std::string& setName, const mongo::HostAndPort primary,
                   const std::vector<std::pair<std::string, bool>> membersAndHealths, const std::string errorStr = "");

        ReplicaSet() {};

        const std::string setName;
        const mongo::HostAndPort primary;
        const std::vector<std::pair<std::string, bool>> membersAndHealths;
        const std::string errorStr;
    };

}