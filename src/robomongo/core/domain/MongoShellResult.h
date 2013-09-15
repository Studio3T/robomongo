#pragma once
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoDocument.h"

namespace Robomongo
{
    class MongoShellResult
    {
    public:
        typedef std::vector<MongoDocumentPtr> MongoDocumentPtrContainerType;
        MongoShellResult(const std::string &type, const std::string &response, const MongoDocumentPtrContainerType &documents,
                         const MongoQueryInfo &queryInfo, qint64 elapsedms);

        std::string response() const { return _response; }
        std::string type() const { return _type; }
        MongoDocumentPtrContainerType documents() const { return _documents; }
        MongoQueryInfo queryInfo() const { return _queryInfo; }
        qint64 elapsedMs() const { return _elapsedms; }

    private:
        std::string _type;
        std::string _response;
        MongoDocumentPtrContainerType _documents;
        MongoQueryInfo _queryInfo;
        qint64 _elapsedms;
    };

    class MongoShellExecResult
    {
    public:
        MongoShellExecResult() { }
        MongoShellExecResult(const std::vector<MongoShellResult> &results,
                             const std::string &currentServer, bool isCurrentServerValid,
                             const std::string &currentDatabase, bool isCurrentDatabaseValid);

        std::vector<MongoShellResult> results() const { return _results; }
        std::string currentServer() const { return _currentServer; }
        std::string currentDatabase() const { return _currentDatabase; }
        bool isCurrentServerValid() const { return _isCurrentServerValid; }
        bool isCurrentDatabaseValid() const { return _isCurrentDatabaseValid; }

    private:
        std::vector<MongoShellResult> _results;
        std::string _currentServer;
        std::string _currentDatabase;
        bool _isCurrentServerValid;
        bool _isCurrentDatabaseValid;
    };
}
