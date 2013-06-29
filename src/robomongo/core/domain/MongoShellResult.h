#pragma once

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class MongoShellResult
    {
    public:
        MongoShellResult(const QString &type, const QString &response, const QList<MongoDocumentPtr> &documents,
                         const MongoQueryInfo &queryInfo, qint64 elapsedms);

        QString response() const { return _response; }
        QString type() const { return _type; }
        QList<MongoDocumentPtr> documents() const { return _documents; }
        MongoQueryInfo queryInfo() const { return _queryInfo; }
        qint64 elapsedMs() const { return _elapsedms; }

    private:
        QString _type;
        QString _response;
        QList<MongoDocumentPtr> _documents;
        MongoQueryInfo _queryInfo;
        qint64 _elapsedms;
    };

    class MongoShellExecResult
    {
    public:
        MongoShellExecResult() { }
        MongoShellExecResult(const QList<MongoShellResult> &results,
                             const QString &currentServer, bool isCurrentServerValid,
                             const QString &currentDatabase, bool isCurrentDatabaseValid);

        QList<MongoShellResult> results() const { return _results; }
        QString currentServer() const { return _currentServer; }
        QString currentDatabase() const { return _currentDatabase; }
        bool isCurrentServerValid() const { return _isCurrentServerValid; }
        bool isCurrentDatabaseValid() const { return _isCurrentDatabaseValid; }

    private:
        QList<MongoShellResult> _results;
        QString _currentServer;
        QString _currentDatabase;
        bool _isCurrentServerValid;
        bool _isCurrentDatabaseValid;
    };
}
