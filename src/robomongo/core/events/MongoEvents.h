#pragma once

#include <QString>
#include <QStringList>
#include <QEvent>
#include <mongo/client/dbclient.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/Event.h"

namespace Robomongo
{
    /**
     * @brief Init Request & Response
     */

    class InitRequest : public Event
    {
        R_EVENT

        InitRequest(QObject *sender) :
            Event(sender) {}
    };

    class InitResponse : public Event
    {
        R_EVENT

        InitResponse(QObject *sender) :
            Event(sender) {}

        InitResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };

    /**
     * @brief Init Request & Response
     */

    class FinalizeRequest : public Event
    {
        R_EVENT

        FinalizeRequest(QObject *sender) :
            Event(sender) {}
    };

    class FinalizeResponse : public Event
    {
        R_EVENT

        FinalizeResponse(QObject *sender) :
            Event(sender) {}

        FinalizeResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };


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

        EstablishConnectionResponse(QObject *sender, const QString &address) :
            Event(sender),
            address(address) {}

        EstablishConnectionResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        QString address;
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

        LoadDatabaseNamesResponse(QObject *sender, const QStringList &databaseNames) :
            Event(sender),
            databaseNames(databaseNames) {}

        LoadDatabaseNamesResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        QStringList databaseNames;
    };


    /**
     * @brief LoadCollectionNames
     */

    class LoadCollectionNamesRequest : public Event
    {
        R_EVENT

    public:
        LoadCollectionNamesRequest(QObject *sender, const QString &databaseName) :
            Event(sender),
            _databaseName(databaseName) {}

        QString databaseName() const { return _databaseName; }

    private:
        QString _databaseName;
    };

    class LoadCollectionNamesResponse : public Event
    {
        R_EVENT

    public:
        LoadCollectionNamesResponse(QObject *sender, const QString &databaseName,
                                    const QList<MongoCollectionInfo> &collectionInfos) :
            Event(sender),
            _databaseName(databaseName),
            _collectionInfos(collectionInfos) { }

        LoadCollectionNamesResponse(QObject *sender, EventError error) :
            Event(sender, error) {}

        QString databaseName() const { return _databaseName; }
        QList<MongoCollectionInfo> collectionInfos() const { return _collectionInfos; }

    private:
        QString _databaseName;
        QList<MongoCollectionInfo> _collectionInfos;
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

        ExecuteQueryResponse(QObject *sender, int resultIndex, const MongoQueryInfo &queryInfo, const QList<MongoDocumentPtr> &documents) :
            Event(sender),
            resultIndex(resultIndex),
            queryInfo(queryInfo),
            documents(documents) { }

        ExecuteQueryResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        int resultIndex;
        MongoQueryInfo queryInfo;
        QList<MongoDocumentPtr> documents;
    };


    /**
     * @brief ExecuteScript
     */

    class ExecuteScriptRequest : public Event
    {
        R_EVENT

        ExecuteScriptRequest(QObject *sender, const QString &script, const QString &dbName, int take = 0, int skip = 0) :
            Event(sender),
            script(script),
            databaseName(dbName),
            take(take),
            skip(skip) {}

        QString script;
        QString databaseName;
        int take; //
        int skip;
    };

    class ExecuteScriptResponse : public Event
    {
        R_EVENT

        ExecuteScriptResponse(QObject *sender, const MongoShellExecResult &result, bool empty) :
            Event(sender),
            result(result),
            empty(empty) { }

        ExecuteScriptResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        MongoShellExecResult result;
        bool empty;
    };



    class SomethingHappened : public Event
    {
        R_EVENT

        SomethingHappened(QObject *sender, const QString &something) :
            Event(sender),
            something(something) { }

        QString something;
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

        OpeningShellEvent(QObject *sender, MongoShell *shell, const QString &initialScript,
                          const QString &shellName = QString()) :
            Event(sender),
            shell(shell),
            initialScript(initialScript),
            shellName(shellName) { }

        MongoShell *shell;
        QString initialScript;
        QString shellName;
    };

    class ConnectionFailedEvent : public Event
    {
        R_EVENT

        ConnectionFailedEvent(MongoServer *server) :
            Event((QObject *)server),
            server(server) { }

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

        DocumentListLoadedEvent(QObject *sender, int resultIndex, const MongoQueryInfo &queryInfo, const QString &query, const QList<MongoDocumentPtr> &list) :
            Event(sender),
            resultIndex(resultIndex),
            queryInfo(queryInfo),
            query(query),
            list(list) { }

        int resultIndex;
        MongoQueryInfo queryInfo;
        QList<MongoDocumentPtr> list;
        QString query;
    };

    class ScriptExecutedEvent : public Event
    {
        R_EVENT

        ScriptExecutedEvent(QObject *sender, const MongoShellExecResult &result, bool empty) :
            Event(sender),
            result(result),
            empty(empty) { }

        MongoShellExecResult result;
        bool empty;
    };

}
