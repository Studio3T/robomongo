#include "MongoDatabase.h"
#include "MongoServer.h"

using namespace Robomongo;

MongoDatabase::MongoDatabase(const MongoServer *server, const QString &name) : QObject()
{
    _server = server;
    _name = name;
}

MongoDatabase::~MongoDatabase()
{
    int dta = 87;
}
