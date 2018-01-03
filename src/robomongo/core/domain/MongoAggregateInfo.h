#pragma once

namespace Robomongo
{
    struct AggrInfo
    {
        AggrInfo() {}

        AggrInfo& operator=(const AggrInfo& aggrInfo) 
        {
            collectionName = aggrInfo.collectionName;
            skip = aggrInfo.skip;
            batchSize = aggrInfo.batchSize;
            isValid = aggrInfo.isValid;
            return *this;
        };

        AggrInfo(const std::string& collectionName, int skip, int batchSize) :
            collectionName(collectionName), skip(skip), batchSize(batchSize), isValid(true)
        {}

        std::string collectionName = "";
        int skip = 0;
        int batchSize = 0;      
        bool isValid = false;
    };
}
