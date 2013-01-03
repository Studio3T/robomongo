#ifndef MONGOEVENTS_H
#define MONGOEVENTS_H

#include <QString>
#include <QStringList>
#include <QEvent>
#include "Core.h"
#include "mongo/client/dbclient.h"
#include <engine/Result.h>
#include "domain/MongoShellResult.h"
#include "Event.h"

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

        LoadCollectionNamesRequest(QObject *sender, const QString &databaseName) :
            Event(sender),
            databaseName(databaseName) {}

        QString databaseName;
    };

    class LoadCollectionNamesResponse : public Event
    {
        R_EVENT

        LoadCollectionNamesResponse(QObject *sender, const QString &databaseName,
                                    const QStringList &collectionNames) :
            Event(sender),
            databaseName(databaseName),
            collectionNames(collectionNames) { }

        LoadCollectionNamesResponse(QObject *sender, EventError error) :
            Event(sender, error) {}

        QString databaseName;
        QStringList collectionNames;
    };


    /**
     * @brief Query Mongodb
     */

    class ExecuteQueryRequest : public Event
    {
        R_EVENT

        ExecuteQueryRequest(QObject *sender, const QString &nspace, int take = 0, int skip = 0) :
            Event(sender),
            nspace(nspace),
            take(take),
            skip(skip) {}

        QString nspace; //namespace of object (i.e. "database_name.collection_name")
        int take; //
        int skip;
    };

    class ExecuteQueryResponse : public Event
    {
        R_EVENT

        ExecuteQueryResponse(QObject *sender, const QList<mongo::BSONObj> &documents) :
            Event(sender),
            documents(documents) { }

        ExecuteQueryResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        QList<mongo::BSONObj> documents;
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

        ExecuteScriptResponse(QObject *sender, const ExecResult &result) :
            Event(sender),
            result(result) { }

        ExecuteScriptResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        ExecResult result;
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

        DocumentListLoadedEvent(QObject *sender, const QString &query, const QList<MongoDocumentPtr> &list) :
            Event(sender),
            query(query),
            list(list) { }

        QList<MongoDocumentPtr> list;
        QString query;
    };

    class ScriptExecutedEvent : public Event
    {
        R_EVENT

        ScriptExecutedEvent(QObject *sender, const MongoShellExecResult &result) :
            Event(sender),
            result(result) { }

        MongoShellExecResult result;
    };

}

#endif // MONGOEVENTS_H
