#pragma once

#include "robomongo/core/Core.h"
#include "robomongo/core/engine/Result.h"
#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class MongoShellResult
    {
    public:
        MongoShellResult(const QString &response, const QList<MongoDocumentPtr> &documents,
                         const MongoQueryInfo &queryInfo, qint64 elapsedms) :
            response(response),
            documents(documents),
            queryInfo(queryInfo),
            elapsedms(elapsedms) { }

        QString response;
        QList<MongoDocumentPtr> documents;
        MongoQueryInfo queryInfo;
        qint64 elapsedms;
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
