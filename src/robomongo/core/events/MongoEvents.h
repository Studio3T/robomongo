#pragma once

#include <QString>
#include <QStringList>
#include <QEvent>
#include <mongo/client/dbclientinterface.h>

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/CursorPosition.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/events/MongoEventsInfo.h"
#include "robomongo/core/Event.h"

namespace Robomongo
{
    class MongoServer;
    class MongoShell;
    class MongoDatabase;
    class MongoWorker;

    /**
     * @brief EstablishConnection
     */

    class EstablishConnectionRequest : public Event
    {
        R_EVENT

        EstablishConnectionRequest(QObject *sender, const QString &databaseName,
                                   const QString &userName, const QString &userPassword) :
            Event(sender),
            databaseName(databaseName),
            userName(userName),
            userPassword(userPassword) {}

        QString databaseName;
        QString userName;
        QString userPassword;
    };

    class EstablishConnectionResponse : public Event
    {
        R_EVENT

        EstablishConnectionResponse(QObject *sender, const ConnectionInfo &info, const ErrorInfo &error) :
            Event(sender,error),
            _info(info) {}

        const ConnectionInfo &info() const{
            return _info;
        }
        const ConnectionInfo _info;
    };


    /**
     * @brief LoadDatabaseNames
     */

    class LoadDatabaseNamesRequest : public Event
    {
        R_EVENT

        LoadDatabaseNamesRequest(QObject *sender) :
            Event(sender) {}
    };

    class LoadDatabaseNamesResponse : public Event
    {
        R_EVENT

        LoadDatabaseNamesResponse(QObject *sender, const std::vector<std::string> &databaseNames, const ErrorInfo &error) :
            Event(sender,error),
            databaseNames(databaseNames) {}

        std::vector<std::string> databaseNames;
    };

    /**
     * @brief InsertDocument
     */

    class InsertDocumentRequest : public Event
    {
        R_EVENT

    public:
        InsertDocumentRequest(QObject *sender, const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite = false) :
            Event(sender),
            _obj(obj),
            _ns(ns),
            _overwrite(overwrite) {}

        mongo::BSONObj obj() const { return _obj; }
        MongoNamespace ns() const { return _ns; }
        bool overwrite() const { return _overwrite; }

    private:
        mongo::BSONObj _obj;
        const MongoNamespace _ns;
        bool _overwrite;
    };

    class InsertDocumentResponse : public Event
    {
        R_EVENT

    public:
        InsertDocumentResponse(QObject *sender, const ErrorInfo &error) :
            Event(sender, error) {}
    };

    /**
     * @brief Remove Document
     */

    class RemoveDocumentRequest : public Event
    {
        R_EVENT

    public:
        RemoveDocumentRequest(QObject *sender, mongo::Query query, const MongoNamespace &ns, bool justOne = true) :
            Event(sender),
            _query(query),
            _ns(ns),
            _justOne(justOne) {}

        mongo::Query query() const { return _query; }
        MongoNamespace ns() const { return _ns; }
        bool justOne() const { return _justOne; }

    private:
        mongo::Query _query;
        const MongoNamespace _ns;
        bool _justOne;
    };

    class RemoveDocumentResponse : public Event
    {
        R_EVENT

    public:

        RemoveDocumentResponse(QObject *sender, const ErrorInfo &error) :
            Event(sender, error) {}
    };

    /**
     * @brief Create Database
     */

    class CreateDatabaseRequest : public Event
    {
        R_EVENT

    public:
        CreateDatabaseRequest(QObject *sender, const std::string &database) :
            Event(sender),
            _database(database) {}

        std::string database() const { return _database; }

    private:
        std::string _database;
    };

    class CreateDatabaseResponse : public Event
    {
        R_EVENT

    public:
        CreateDatabaseResponse(QObject *sender, const ErrorInfo &error) :
            Event(sender, error) {}
    };



    /**
     * @brief Drop Database
     */

    class DropDatabaseRequest : public Event
    {
        R_EVENT

    public:
        DropDatabaseRequest(QObject *sender, const std::string &database) :
            Event(sender),
            _database(database) {}

        std::string database() const { return _database; }

    private:
        std::string _database;
    };

    class DropDatabaseResponse : public Event
    {
        R_EVENT

    public:
         DropDatabaseResponse(QObject *sender, const ErrorInfo &error) :
            Event(sender, error) {}
    };

    /**
     * @brief Query Mongodb
     */

    class ExecuteQueryRequest : public Event
    {
        R_EVENT

    public:
        ExecuteQueryRequest(QObject *sender, int resultIndex, const MongoQueryInfo &queryInfo) :
            Event(sender),
            _resultIndex(resultIndex),
            _queryInfo(queryInfo) {}

        int resultIndex() const { return _resultIndex; }
        MongoQueryInfo queryInfo() const { return _queryInfo; }

    private:
        int _resultIndex; //external user data;
        MongoQueryInfo _queryInfo;
    };

    class ExecuteQueryResponse : public Event
    {
        R_EVENT

        ExecuteQueryResponse(QObject *sender, int resultIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents, const ErrorInfo &error) :
            Event(sender, error),
            resultIndex(resultIndex),
            queryInfo(queryInfo),
            documents(documents) { }

        int resultIndex;
        MongoQueryInfo queryInfo;
        std::vector<MongoDocumentPtr> documents;
    };

    class AutocompleteRequest : public Event
    {
        R_EVENT

        AutocompleteRequest(QObject *sender, const std::string &prefix) :
            Event(sender),
            prefix(prefix) {}

        std::string prefix;
    };

    class AutocompleteResponse : public Event
    {
        R_EVENT

        AutocompleteResponse(QObject *sender,const QStringList &list, const std::string &prefix, const ErrorInfo &error) :
            Event(sender, error),
            list(list),
            prefix(prefix) {}

        QStringList list;
        std::string prefix;
    };


    /**
     * @brief ExecuteScript
     */

    class ExecuteScriptRequest : public Event
    {
        R_EVENT

        ExecuteScriptRequest(QObject *sender, const std::string &script, const std::string &dbName, int take = 0, int skip = 0) :
            Event(sender),
            script(script),
            databaseName(dbName),
            take(take),
            skip(skip) {}

        std::string script;
        std::string databaseName;
        int take; //
        int skip;
    };

    class ExecuteScriptResponse : public Event
    {
        R_EVENT

        ExecuteScriptResponse(QObject *sender, const MongoShellExecResult &result, bool empty, const ErrorInfo &error) :
            Event(sender, error),
            result(result),
            empty(empty) { }

        MongoShellExecResult result;
        bool empty;
    };

    class ConnectingEvent : public Event
    {
        R_EVENT

        ConnectingEvent(QObject *sender, MongoServer *server) :
            Event(sender),
            server(server) { }

        MongoServer *server;
    };

    class OpeningShellEvent : public Event
    {
        R_EVENT

        OpeningShellEvent(QObject *sender, MongoShell *shell) :
            Event(sender),
            shell(shell) {}

        MongoShell *shell;
    };

    class ConnectionFailedEvent : public Event
    {
        R_EVENT

        ConnectionFailedEvent(MongoServer *server) :
            Event((QObject *)server),
            server(server) { }

        ConnectionFailedEvent(MongoServer *server, const ErrorInfo &error) :
            Event((QObject *)server, error),
            server(server) {}

        MongoServer *server;
    };

//    class ScriptExecute

    class ConnectionEstablishedEvent : public Event
    {
        R_EVENT

        ConnectionEstablishedEvent(MongoServer *server) :
            Event((QObject *)server),
            server(server) { }

        MongoServer *server;
    };

    class DatabaseListLoadedEvent : public Event
    {
        R_EVENT

        DatabaseListLoadedEvent(QObject *sender, const QList<MongoDatabase *> &list) :
            Event(sender),
            list(list) { }

        QList<MongoDatabase *> list;
    };

    class DocumentListLoadedEvent : public Event
    {
        R_EVENT

    public:
        DocumentListLoadedEvent(QObject *sender, int resultIndex, const MongoQueryInfo &queryInfo, const std::string &query, const std::vector<MongoDocumentPtr> &docs) :
            Event(sender),
            _resultIndex(resultIndex),
            _queryInfo(queryInfo),
            _query(query),
            _documents(docs) { }

        int resultIndex() const { return _resultIndex; }
        MongoQueryInfo queryInfo() const { return _queryInfo; }
        std::vector<MongoDocumentPtr> documents() const { return _documents; }
        std::string query() const { return _query; }

    private:
        int _resultIndex;
        MongoQueryInfo _queryInfo;
        std::vector<MongoDocumentPtr> _documents;
        std::string _query;
    };

    class ScriptExecutedEvent : public Event
    {
        R_EVENT

    public:
        ScriptExecutedEvent(QObject *sender, const MongoShellExecResult &result, bool empty) :
            Event(sender),
            _result(result),
            _empty(empty) { }

        MongoShellExecResult result() const { return _result; }
        bool empty() const { return _empty; }

    private:
        MongoShellExecResult _result;
        bool _empty;
    };

    class ScriptExecutingEvent : public Event
    {
        R_EVENT

    public:
        ScriptExecutingEvent(QObject *sender) :
            Event(sender) { }
    };

    class QueryWidgetUpdatedEvent : public Event
    {
        R_EVENT

    public:
        QueryWidgetUpdatedEvent(QObject *sender, int numOfResults) :
            Event(sender),
          _numOfResults(numOfResults) { }

        int numOfResults() const { return _numOfResults; }

    private:
        int _numOfResults;
    };
}
