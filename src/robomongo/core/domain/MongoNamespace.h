#pragma once
#include <string>

namespace Robomongo
{
    class MongoNamespace
    {
    public:
        MongoNamespace(const std::string &ns);
        MongoNamespace(const std::string &database, const std::string &collection);
        MongoNamespace() {}
        std::string toString() const { return _ns; }
        std::string databaseName() const { return _databaseName; }
        std::string collectionName() const { return _collectionName; }

    private:
        std::string _ns;
        std::string _databaseName;
        std::string _collectionName;
    };
}
