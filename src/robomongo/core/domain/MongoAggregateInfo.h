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
            pipeline = aggrInfo.pipeline;
            options = aggrInfo.options;
            isValid = aggrInfo.isValid;
            resultIndex = aggrInfo.resultIndex;
            return *this;
        };

        AggrInfo(const std::string& collectionName, int skip, int batchSize, 
                 mongo::BSONObj const& pipeline, mongo::BSONObj const& options, int resultIndex) :
            collectionName(collectionName), skip(skip), batchSize(batchSize), pipeline(pipeline), 
            options(options), isValid(true), resultIndex(resultIndex)
        {}

        std::string collectionName = "";
        int skip = 0;
        int batchSize = 0;   
        mongo::BSONObj pipeline;
        mongo::BSONObj options;
        bool isValid = false;
        int resultIndex = -1;
    };
}
