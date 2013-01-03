#ifndef MONGOSHELLRESULT_H
#define MONGOSHELLRESULT_H

#include <engine/Result.h>
#include "Core.h"

namespace Robomongo
{
    class MongoShellResult
    {
    public:
        MongoShellResult(const QString &response, const QList<MongoDocumentPtr> &documents, const QString &databaseName);
        QList<MongoDocumentPtr> documents;
        QString response;
        QString databaseName;

        static QList<MongoShellResult> fromResult(QList<Result> result);
    };

    class MongoShellExecResult
    {
    public:
        MongoShellExecResult() {}
        MongoShellExecResult(const QList<MongoShellResult> &results, const QString &currentDatabase) :
            results(results),
            currentDatabase(currentDatabase) {}

        QList<MongoShellResult> results;
        QString currentDatabase;
    };
}

#endif // MONGOSHELLRESULT_H
