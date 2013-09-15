#include "robomongo/core/domain/MongoShellResult.h"

namespace Robomongo
{
    MongoShellResult::MongoShellResult(const std::string &type, const std::string &response, const MongoDocumentPtrContainerType &documents,
                     const MongoQueryInfo &queryInfo, qint64 elapsedms) :
        _type(type),
        _response(response),
        _documents(documents),
        _queryInfo(queryInfo),
        _elapsedms(elapsedms) { }

    MongoShellExecResult::MongoShellExecResult(const std::vector<MongoShellResult> &results,
                         const std::string &currentServer, bool isCurrentServerValid,
                         const std::string &currentDatabase, bool isCurrentDatabaseValid) :
        _results(results),
        _currentServer(currentServer),
        _currentDatabase(currentDatabase),
        _isCurrentServerValid(isCurrentServerValid),
        _isCurrentDatabaseValid(isCurrentDatabaseValid) { }
}
