#include "MongoShellResult.h"
#include "domain/MongoDocument.h"

using namespace Robomongo;

MongoShellResult::MongoShellResult(const QString &response, const QList<MongoDocumentPtr> &documents, const QString &serverName, bool isServerValid,
                                   const QString &databaseName, bool isDatabaseValid, const QString &collectionName, bool isCollectionValid) :
    response(response),
    documents(documents),
    serverName(serverName),
    isServerValid(isServerValid),
    databaseName(databaseName),
    isDatabaseValid(isDatabaseValid),
    collectionName(collectionName),
    isCollectionValid(isCollectionValid) { }

QList<MongoShellResult> MongoShellResult::fromResult(QList<Result> results)
{
    QList<MongoShellResult> shellResults;

    foreach(Result result, results) {
        QList<MongoDocumentPtr> list;
        foreach(mongo::BSONObj obj, result.documents) {
            MongoDocumentPtr doc(new MongoDocument(obj));
            list.append(doc);
        }

        shellResults.append(MongoShellResult(result.response, list,
                                             result.serverName, result.isServerValid,
                                             result.databaseName, result.isDatabaseValid,
                                             result.collectionName, result.isCollectionValid));
    }

    return shellResults;
}

