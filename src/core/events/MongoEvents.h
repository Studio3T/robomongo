#ifndef MONGOEVENTS_H
#define MONGOEVENTS_H

#include <QString>
#include <QStringList>
#include <QEvent>
#include "Core.h"
#include "mongo/client/dbclient.h"

#define R_MESSAGE \
    public: \
        const static QEvent::Type EventType;


namespace Robomongo
{
    /**
     * @brief Error class
     */
    class Error
    {
    public:
        Error() :
            isNull(true) {}

        Error(const QString &errorMessage) :
            isNull(false),
            errorMessage(errorMessage) {}

        QString errorMessage;
        bool isNull;
    };


    /**
     * @brief Request & Response
     */

    class Request : public QEvent
    {
    public:
        Request(QEvent::Type type, QObject *sender) :
            QEvent(type),
            sender(sender) {}

        QObject *sender;
    };

    class Response : public QEvent
    {
    public:
        Response(QEvent::Type type) :
            QEvent(type) {}

        Response(QEvent::Type type, Error error) :
            QEvent(type),
            error(error) {}

        bool isError() const { return !error.isNull; }
        QString errorMessage() const { return error.errorMessage; }

        Error error;
    };

    /**
     * @brief Init Request & Response
     */

    class InitRequest : public Request
    {
        R_MESSAGE

        InitRequest(QObject *sender) :
            Request(EventType, sender) {}
    };

    class InitResponse : public Response
    {
        R_MESSAGE

        InitResponse() :
            Response(EventType) {}

        InitResponse(const Error &error) :
            Response(EventType, error) {}
    };


    /**
     * @brief EstablishConnection
     */

    class EstablishConnectionRequest : public Request
    {
        R_MESSAGE

        EstablishConnectionRequest(QObject *sender, const QString &databaseName,
                                   const QString &userName, const QString &userPassword) :
            Request(EventType, sender),
            databaseName(databaseName),
            userName(userName),
            userPassword(userPassword) {}

        QString databaseName;
        QString userName;
        QString userPassword;
    };

    class EstablishConnectionResponse : public Response
    {
        R_MESSAGE

        EstablishConnectionResponse(const QString &address) :
            Response(EventType),
            address(address) {}

        EstablishConnectionResponse(const Error &error) :
            Response(EventType, error) {}

        QString address;
    };


    /**
     * @brief LoadDatabaseNames
     */

    class LoadDatabaseNamesRequest : public Request
    {
        R_MESSAGE

        LoadDatabaseNamesRequest(QObject *sender) :
            Request(EventType, sender) {}
    };

    class LoadDatabaseNamesResponse : public Response
    {
        R_MESSAGE

        LoadDatabaseNamesResponse(const QStringList &databaseNames) :
            Response(EventType),
            databaseNames(databaseNames) {}

        LoadDatabaseNamesResponse(const Error &error) :
            Response(EventType, error) {}

        QStringList databaseNames;
    };


    /**
     * @brief LoadCollectionNames
     */

    class LoadCollectionNamesRequest : public Request
    {
        R_MESSAGE

        LoadCollectionNamesRequest(QObject *sender, const QString &databaseName) :
            Request(EventType, sender),
            databaseName(databaseName) {}

        QString databaseName;
    };

    class LoadCollectionNamesResponse : public Response
    {
        R_MESSAGE

        LoadCollectionNamesResponse(const QString &databaseName, const QStringList &collectionNames) :
            Response(EventType),
            databaseName(databaseName),
            collectionNames(collectionNames) { }

        LoadCollectionNamesResponse(const Error &error) :
            Response(EventType, error) {}

        QString databaseName;
        QStringList collectionNames;
    };


    /**
     * @brief Query Mongodb
     */

    class ExecuteQueryRequest : public Request
    {
        R_MESSAGE

        ExecuteQueryRequest(QObject *sender, const QString &nspace, int take = 0, int skip = 0) :
            Request(EventType, sender),
            nspace(nspace),
            take(take),
            skip(skip) {}

        QString nspace; //namespace of object (i.e. "database_name.collection_name")
        int take; //
        int skip;
    };

    class ExecuteQueryResponse : public Response
    {
        R_MESSAGE

        ExecuteQueryResponse(const QList<mongo::BSONObj> &documents) :
            Response(EventType),
            documents(documents) { }

        ExecuteQueryResponse(const Error &error) :
            Response(EventType, error) {}

        QList<mongo::BSONObj> documents;
    };


    /**
     * @brief ExecuteScript
     */

    class ExecuteScriptRequest : public Request
    {
        R_MESSAGE

        ExecuteScriptRequest(QObject *sender, const QString &script, int take = 0, int skip = 0) :
            Request(EventType, sender),
            script(script),
            take(take),
            skip(skip) {}

        QString script;
        int take; //
        int skip;
    };

    class ExecuteScriptResponse : public Response
    {
        R_MESSAGE

        ExecuteScriptResponse(const QList<mongo::BSONObj> &documents) :
            Response(EventType),
            documents(documents),
            hasDocuments(true),
            hasResponse(false) { }

        ExecuteScriptResponse(const QString &response = QString()) :
            Response(EventType),
            response(response),
            hasDocuments(false),
            hasResponse(true) { }

        ExecuteScriptResponse(const Error &error) :
            Response(EventType, error) {}

        QList<mongo::BSONObj> documents;
        QString response;
        bool hasDocuments;
        bool hasResponse;
    };



    class SomethingHappened : public QEvent
    {
        R_MESSAGE

        SomethingHappened(const QString &something) :
            QEvent(EventType),
            something(something) { }

        QString something;
    };

    class ConnectingEvent : public QEvent
    {
        R_MESSAGE

        ConnectingEvent(const MongoServerPtr &server) :
            QEvent(EventType),
            server(server) { }

        MongoServerPtr server;
    };

    class OpeningShellEvent : public QEvent
    {
        R_MESSAGE

        OpeningShellEvent(const MongoShellPtr &shell) :
            QEvent(EventType),
            shell(shell) { }

        MongoShellPtr shell;
    };

    class ConnectionFailedEvent : public QEvent
    {
        R_MESSAGE

        ConnectionFailedEvent(const MongoServerPtr &server) :
            QEvent(EventType),
            server(server) { }

        MongoServerPtr server;
    };

    class ConnectionEstablishedEvent : public QEvent
    {
        R_MESSAGE

        ConnectionEstablishedEvent(const MongoServerPtr &server) :
            QEvent(EventType),
            server(server) { }

        MongoServerPtr server;
    };

    class DatabaseListLoadedEvent : public QEvent
    {
        R_MESSAGE

        DatabaseListLoadedEvent(const QList<MongoDatabasePtr> &list) :
            QEvent(EventType),
            list(list) { }

        QList<MongoDatabasePtr> list;
    };

    class CollectionListLoadedEvent : public QEvent
    {
        R_MESSAGE

        CollectionListLoadedEvent(const QList<MongoCollectionPtr> &list) :
            QEvent(EventType),
            list(list) { }

        QList<MongoCollectionPtr> list;
    };

    class DocumentListLoadedEvent : public QEvent
    {
        R_MESSAGE

        DocumentListLoadedEvent(const QList<MongoDocumentPtr> &list) :
            QEvent(EventType),
            list(list) { }

        QList<MongoDocumentPtr> list;
    };

    class ScriptExecutedEvent : public QEvent
    {
        R_MESSAGE

        ScriptExecutedEvent(const QList<MongoDocumentPtr> &list) :
            QEvent(EventType),
            documents(list),
            hasDocuments(true),
            hasResponse(false) { }

        ScriptExecutedEvent(const QString &response) :
            QEvent(EventType),
            response(response),
            hasDocuments(false),
            hasResponse(true) { }

        QList<MongoDocumentPtr> documents;
        QString response;
        bool hasDocuments;
        bool hasResponse;
    };

}

#endif // MONGOEVENTS_H
