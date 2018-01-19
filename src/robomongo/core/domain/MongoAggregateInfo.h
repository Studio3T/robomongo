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
            return *this;
        };

        AggrInfo(const std::string& collectionName, int skip, int batchSize, 
                 mongo::BSONObj const& pipeline, mongo::BSONObj const& options) :
            collectionName(collectionName), skip(skip), batchSize(batchSize), pipeline(pipeline), 
            options(options), isValid(true)
        {}

        std::string collectionName = "";
        int skip = 0;
        int batchSize = 0;   
        mongo::BSONObj pipeline;
        mongo::BSONObj options;
        bool isValid = false;
    };
}
