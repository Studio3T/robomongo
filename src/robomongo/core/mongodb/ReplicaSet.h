#pragma once

#include <mongo/util/net/hostandport.h>

namespace Robomongo
{
    struct ReplicaSet
    {
        ReplicaSet(const std::string& setName, const mongo::HostAndPort primary,
                   const std::vector<std::pair<std::string, bool>> membersAndHealths, const std::string errorStr = "");

        ReplicaSet() {};

        std::string const setName;
        mongo::HostAndPort const primary;
        // pair: {HostNameAndPort, Health}
        std::vector<std::pair<std::string, bool>> const membersAndHealths;
        std::string const errorStr;
    };

}