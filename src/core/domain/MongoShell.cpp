#include "MongoShell.h"
#include "MongoCollection.h"

using namespace Robomongo;

MongoShell::MongoShell(QObject *parent) :
    QObject(parent)
{
}

void MongoShell::open(const MongoCollectionPtr &collection)
{
}
