#pragma once
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoAggregateInfo.h"
#include "robomongo/core/domain/MongoDocument.h"

namespace Robomongo
{
    /* --------------  MongoShellResult Class --------- */
    class MongoShellResult
    {
    public:
        typedef std::vector<MongoDocumentPtr> MongoDocumentPtrContainerType;
        MongoShellResult(
            const std::string &type, const std::string &response,
            const MongoDocumentPtrContainerType &documents,
            const MongoQueryInfo &queryInfo, const std::string &statement,
            qint64 elapsedms, AggrInfo aggrInfo = AggrInfo()) :
            _type(type),
            _response(response),
            _documents(documents),
            _queryInfo(queryInfo),
            _statement(statement),
            _elapsedms(elapsedms),
            _aggrInfo(aggrInfo)
        { }

        std::string response() const { return _response; }
        std::string type() const { return _type; }
        MongoDocumentPtrContainerType documents() const { return _documents; }
        MongoQueryInfo queryInfo() const { return _queryInfo; }
        std::string statement() const { return _statement; }
        std::string statementShort() const {
            std::size_t const LEN = _statement.size() < 10 ? _statement.size() : 10;
            std::string statementShort { _statement, 0, LEN };
            statementShort.append((_statement.size() > 10) ? ".." : "");
            return statementShort;
        }

        qint64 elapsedMs() const { return _elapsedms; }
        AggrInfo const& aggrInfo() const { return _aggrInfo; }

    private:
        std::string _type;
        std::string _response;
        MongoDocumentPtrContainerType _documents;
        MongoQueryInfo _queryInfo;
        std::string const _statement;
        qint64 _elapsedms;
        AggrInfo _aggrInfo = AggrInfo();
    };

    /* --------------  MongoShellExecResult Class --------- */
    class MongoShellExecResult
    {
    public:
        MongoShellExecResult() { }

        MongoShellExecResult(
            const std::vector<MongoShellResult> &results,
            const std::string &currentServer, bool isCurrentServerValid,
            const std::string &currentDatabase, bool isCurrentDatabaseValid,
            bool timeoutReached = false) :
            _results(results),
            _currentServer(currentServer),
            _currentDatabase(currentDatabase),
            _isCurrentServerValid(isCurrentServerValid),
            _isCurrentDatabaseValid(isCurrentDatabaseValid),
            _timeoutReached(timeoutReached)
        { }

        MongoShellExecResult(bool error, std::string const& errorMsg = "", bool timeoutReached = false) : 
            _error(error), _errorMessage(errorMsg), _timeoutReached(timeoutReached) { }

        std::vector<MongoShellResult> const& results() const { return _results; }
        std::string currentServer() const { return _currentServer; }
        void setCurrentServer(std::string const& server) { _currentServer = server; }
        std::string currentDatabase() const { return _currentDatabase; }        
        bool isCurrentServerValid() const { return _isCurrentServerValid; }
        bool isCurrentDatabaseValid() const { return _isCurrentDatabaseValid; }
        std::string errorMessage() const { return _errorMessage; }
        bool error() const { return _error; }
        bool timeoutReached() const { return _timeoutReached; }

    private:
        std::vector<MongoShellResult> _results;
        std::string _currentServer;
        std::string _currentDatabase;
        bool _isCurrentServerValid;
        bool _isCurrentDatabaseValid;
        std::string _errorMessage;
        bool _error = false;
        bool _timeoutReached = false;
    };
}
