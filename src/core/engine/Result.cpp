#include "Result.h"

using namespace Robomongo;

Result::Result(const QString &response, const QList<mongo::BSONObj> &documents,
               const QString &serverName, bool isServerValid,
               const QString &databaseName, bool isDatabaseValid,
               const QString &collectionName, bool isCollectionValid) :
    response(response),
    documents(documents),
    serverName(serverName),
    isServerValid(isServerValid),
    databaseName(databaseName),
    isDatabaseValid(isDatabaseValid),
    collectionName(collectionName),
    isCollectionValid(isCollectionValid) { }
