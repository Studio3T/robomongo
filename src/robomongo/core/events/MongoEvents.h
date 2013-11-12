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
