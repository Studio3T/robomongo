#pragma once

#include <mongo/util/net/hostandport.h>
#include <vector>

namespace Robomongo
{
    /**
    * @brief Struct to represent live information of a replica set.
    */
    struct ReplicaSet
    {
        ReplicaSet(std::string const& setName, const mongo::HostAndPort primary,
                   std::vector<std::pair<std::string, bool>> const& membersAndHealths, std::string const& errorStr = "");

        ReplicaSet() {};

        std::string const setName;
        mongo::HostAndPort const primary;
        std::vector<std::pair<std::string, bool>> const membersAndHealths;  // pair: {HostNameAndPort, Health}
        std::string const errorStr;
    };
}