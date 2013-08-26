#include "MongoNamespace.h"

#include <stdio.h>

namespace Robomongo
{
    MongoNamespace::MongoNamespace(const std::string &ns) :
        _ns(ns)
    {
        size_t dot = ns.find_first_of('.');
        _collectionName = ns.substr(dot + 1);
        _databaseName = ns.substr(0, dot);
    }

    MongoNamespace::MongoNamespace(const std::string &database, const std::string &collection) :
        _databaseName(database),
        _collectionName(collection)
    {
        char buff[512]={0};
        sprintf(buff,"%s.%s",_databaseName.c_str(), _collectionName.c_str());
        _ns = buff;
    }
}
