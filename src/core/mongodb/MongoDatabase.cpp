#include "MongoDatabase.h"
#include "MongoServer.h"

using namespace Robomongo;

MongoDatabase::MongoDatabase(const MongoServer *server, const QString &name) : QObject()
{
    _server = server;
    _name = name;

    // Check that this is a system database
    _system = name == "admin" ||
              name == "local";
}

MongoDatabase::~MongoDatabase()
{
    int dta = 87;
}
