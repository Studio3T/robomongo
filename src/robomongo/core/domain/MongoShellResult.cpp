#include "robomongo/core/domain/MongoShellResult.h"
namespace Robomongo
{
    MongoShellResult::MongoShellResult(const QString &type, const QString &response, const QList<MongoDocumentPtr> &documents,
                     const MongoQueryInfo &queryInfo, qint64 elapsedms) :
        _type(type),
        _response(response),
        _documents(documents),
        _queryInfo(queryInfo),
        _elapsedms(elapsedms) { }

    MongoShellExecResult::MongoShellExecResult(const QList<MongoShellResult> &results,
                         const QString &currentServer, bool isCurrentServerValid,
                         const QString &currentDatabase, bool isCurrentDatabaseValid) :
        _results(results),
        _currentServer(currentServer),
        _currentDatabase(currentDatabase),
        _isCurrentServerValid(isCurrentServerValid),
        _isCurrentDatabaseValid(isCurrentDatabaseValid) { }
}
