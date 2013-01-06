#ifndef MONGOSHELLRESULT_H
#define MONGOSHELLRESULT_H

#include <engine/Result.h>
#include "Core.h"

namespace Robomongo
{
    class MongoShellResult
    {
    public:
        MongoShellResult(const QString &response, const QList<MongoDocumentPtr> &documents,
                         const QString &serverName, bool isServerValid,
                         const QString &databaseName, bool isDatabaseValid,
                         const QString &collectionName, bool isCollectionValid);

        QList<MongoDocumentPtr> documents;
        QString response;
        QString serverName;
        bool isServerValid;
        QString databaseName;
        bool isDatabaseValid;
        QString collectionName;
        bool isCollectionValid;

        static QList<MongoShellResult> fromResult(QList<Result> result);
    };

    class MongoShellExecResult
    {
    public:
        MongoShellExecResult() { }
        MongoShellExecResult(const QList<MongoShellResult> &results,
                             const QString &currentServer, bool isCurrentServerValid,
                             const QString &currentDatabase, bool isCurrentDatabaseValid) :
            results(results),
            currentServer(currentServer),
            isCurrentServerValid(isCurrentServerValid),
            currentDatabase(currentDatabase),
            isCurrentDatabaseValid(isCurrentDatabaseValid) { }

        QList<MongoShellResult> results;
        QString currentServer;
        bool isCurrentServerValid;
        QString currentDatabase;
        bool isCurrentDatabaseValid;
    };
}

#endif // MONGOSHELLRESULT_H
