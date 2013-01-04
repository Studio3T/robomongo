#ifndef MONGOSHELLRESULT_H
#define MONGOSHELLRESULT_H

#include <engine/Result.h>
#include "Core.h"

namespace Robomongo
{
    class MongoShellResult
    {
    public:
        MongoShellResult(const QString &response, const QList<MongoDocumentPtr> &documents, const QString &databaseName,
                         bool isDatabaseValid);

        QList<MongoDocumentPtr> documents;
        QString response;
        QString databaseName;
        bool isDatabaseValid;

        static QList<MongoShellResult> fromResult(QList<Result> result);
    };

    class MongoShellExecResult
    {
    public:
        MongoShellExecResult() { }
        MongoShellExecResult(const QList<MongoShellResult> &results, const QString &currentDatabase, bool isCurrentDatabaseValid) :
            results(results),
            currentDatabase(currentDatabase),
            isCurrentDatabaseValid(isCurrentDatabaseValid) { }

        QList<MongoShellResult> results;
        QString currentDatabase;
        bool isCurrentDatabaseValid;
    };
}

#endif // MONGOSHELLRESULT_H
