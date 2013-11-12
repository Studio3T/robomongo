#pragma once
#include <string>
#include "robomongo/core/domain/MongoCollectionInfo.h"

namespace Robomongo
{
    struct EnsureIndex
    {
        EnsureIndex(
            const MongoCollectionInfo &collection,
            const std::string &name = std::string(),
            const std::string &request = std::string(),
            bool isUnique = false,
            bool isBackGround = false,
            bool isDropDuplicates = false,
            bool isSparce = false,
            int expireAfter = 0,
            const std::string &defaultLanguage = std::string(),
            const std::string &languageOverride = std::string(),
            const std::string &textWeights = std::string());

        MongoCollectionInfo _collection;
        std::string _name;
        std::string _request;
        bool _unique;
        bool _backGround;
        bool _dropDups;
        bool _sparse;
        int _ttl;
        std::string _defaultLanguage;
        std::string _languageOverride;
        std::string _textWeights;
    };

    struct ConnectionInfo
    {
        ConnectionInfo();
        ConnectionInfo(const std::string &address, const std::vector<std::string> &databases, float version);
        std::string _address;
        std::vector<std::string> _databases;
        float _version;
    };
}
