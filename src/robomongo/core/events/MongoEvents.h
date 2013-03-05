#pragma once

#include <QString>
#include <QStringList>
#include <QEvent>
#include <mongo/client/dbclient.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/domain/CursorPosition.h"
#include "robomongo/core/domain/ScriptInfo.h"
#include "robomongo/core/domain/MongoUser.h"
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
     * @brief LoadCollectionNames
     */

    class LoadUsersRequest : public Event
    {
        R_EVENT

    public:
        LoadUsersRequest(QObject *sender, const QString &databaseName) :
            Event(sender),
            _databaseName(databaseName) {}

        QString databaseName() const { return _databaseName; }

    private:
        QString _databaseName;
    };

    class LoadUsersResponse : public Event
    {
        R_EVENT

    public:
        LoadUsersResponse(QObject *sender, const QString &databaseName,
                                    const QList<MongoUser> &users) :
            Event(sender),
            _databaseName(databaseName),
            _users(users) { }

        LoadUsersResponse(QObject *sender, EventError error) :
            Event(sender, error) {}

        QString databaseName() const { return _databaseName; }
        QList<MongoUser> users() const { return _users; }

    private:
        QString _databaseName;
        QList<MongoUser> _users;
    };


    /**
     * @brief InsertDocument
     */

    class InsertDocumentRequest : public Event
    {
        R_EVENT

    public:
        InsertDocumentRequest(QObject *sender, const mongo::BSONObj &obj, const QString &database, const QString &collection, bool overwrite = false) :
            Event(sender),
            _obj(obj),
            _database(database),
            _collection(collection),
            _overwrite(overwrite) {}

        mongo::BSONObj obj() const { return _obj; }
        QString database() const { return _database; }
        QString collection() const { return _collection; }
        bool overwrite() const { return _overwrite; }

    private:
        mongo::BSONObj _obj;
        QString _database;
        QString _collection;
        bool _overwrite;
    };

    class InsertDocumentResponse : public Event
    {
        R_EVENT

    public:
        InsertDocumentResponse(QObject *sender) :
            Event(sender) {}

        InsertDocumentResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };

    /**
     * @brief Remove Document
     */

    class RemoveDocumentRequest : public Event
    {
        R_EVENT

    public:
        RemoveDocumentRequest(QObject *sender, mongo::Query query, const QString &database, const QString &collection, bool justOne = true) :
            Event(sender),
            _query(query),
            _database(database),
            _collection(collection),
            _justOne(justOne) {}

        mongo::Query query() const { return _query; }
        QString database() const { return _database; }
        QString collection() const { return _collection; }
        bool justOne() const { return _justOne; }

    private:
        mongo::Query _query;
        QString _database;
        QString _collection;
        bool _justOne;
    };

    class RemoveDocumentResponse : public Event
    {
        R_EVENT

    public:
        RemoveDocumentResponse(QObject *sender) :
            Event(sender) {}

        RemoveDocumentResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };

    /**
     * @brief Create Database
     */

    class CreateDatabaseRequest : public Event
    {
        R_EVENT

    public:
        CreateDatabaseRequest(QObject *sender, const QString &database) :
            Event(sender),
            _database(database) {}

        QString database() const { return _database; }

    private:
        QString _database;
    };

    class CreateDatabaseResponse : public Event
    {
        R_EVENT

    public:
        CreateDatabaseResponse(QObject *sender) :
            Event(sender) {}

        CreateDatabaseResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };



    /**
     * @brief Drop Database
     */

    class DropDatabaseRequest : public Event
    {
        R_EVENT

    public:
        DropDatabaseRequest(QObject *sender, const QString &database) :
            Event(sender),
            _database(database) {}

        QString database() const { return _database; }

    private:
        QString _database;
    };

    class DropDatabaseResponse : public Event
    {
        R_EVENT

    public:
        DropDatabaseResponse(QObject *sender) :
            Event(sender) {}

        DropDatabaseResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };

    /**
     * @brief Create Collection
     */

    class CreateCollectionRequest : public Event
    {
        R_EVENT

    public:
        CreateCollectionRequest(QObject *sender, const QString &database, const QString &collection) :
            Event(sender),
            _database(database),
            _collection(collection) {}

        QString database() const { return _database; }
        QString collection() const { return _collection; }

    private:
        QString _database;
        QString _collection;
    };

    class CreateCollectionResponse : public Event
    {
        R_EVENT

    public:
        CreateCollectionResponse(QObject *sender) :
            Event(sender) {}

        CreateCollectionResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };



    /**
     * @brief Drop Collection
     */

    class DropCollectionRequest : public Event
    {
        R_EVENT

    public:
        DropCollectionRequest(QObject *sender, const QString &database,
                              const QString &collection) :
            Event(sender),
            _database(database),
            _collection(collection){}

        QString database() const { return _database; }
        QString collection() const { return _collection; }

    private:
        QString _database;
        QString _collection;
    };

    class DropCollectionResponse : public Event
    {
        R_EVENT

    public:
        DropCollectionResponse(QObject *sender) :
            Event(sender) {}

        DropCollectionResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };

    /**
     * @brief Rename Collection
     */

    class RenameCollectionRequest : public Event
    {
        R_EVENT

    public:
        RenameCollectionRequest(QObject *sender, const QString &database,
                              const QString &collection, const QString &newCollection) :
            Event(sender),
            _database(database),
            _collection(collection),
            _newCollection(newCollection) {}

        QString database() const { return _database; }
        QString collection() const { return _collection; }
        QString newCollection() const { return _newCollection; }

    private:
        QString _database;
        QString _collection;
        QString _newCollection;
    };

    class RenameCollectionResponse : public Event
    {
        R_EVENT

    public:
        RenameCollectionResponse(QObject *sender) :
            Event(sender) {}

        RenameCollectionResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };

    /**
     * @brief Create User
     */

    class CreateUserRequest : public Event
    {
        R_EVENT

    public:
        CreateUserRequest(QObject *sender, const QString &database, const MongoUser &user, bool overwrite = false) :
            Event(sender),
            _database(database),
            _user(user),
            _overwrite(overwrite) {}

        QString database() const { return _database; }
        MongoUser user() const { return _user; }
        bool overwrite() const { return _overwrite; }

    private:
        QString _database;
        MongoUser _user;
        bool _overwrite;
    };

    class CreateUserResponse : public Event
    {
        R_EVENT

    public:
        CreateUserResponse(QObject *sender) :
            Event(sender) {}

        CreateUserResponse(QObject *sender, EventError error) :
            Event(sender, error) {}
    };


    /**
     * @brief Drop User
     */

    class DropUserRequest : public Event
    {
        R_EVENT

    public:
        DropUserRequest(QObject *sender, const QString &database, const mongo::OID &id) :
            Event(sender),
            _database(database),
            _id(id) {}

        QString database() const { return _database; }
        mongo::OID id() const { return _id; }

    private:
        QString _database;
        mongo::OID _id;
    };

    class DropUserResponse : public Event
    {
        R_EVENT

    public:
        DropUserResponse(QObject *sender) :
            Event(sender) {}

        DropUserResponse(QObject *sender, EventError error) :
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

    class AutocompleteRequest : public Event
    {
        R_EVENT

        AutocompleteRequest(QObject *sender, const QString &prefix) :
            Event(sender),
            prefix(prefix) {}

        QString prefix;
    };

    class AutocompleteResponse : public Event
    {
        R_EVENT

        AutocompleteResponse(QObject *sender, const QStringList &list, const QString &prefix) :
            Event(sender),
            list(list),
            prefix(prefix) {}

        QStringList list;
        QString prefix;
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

        OpeningShellEvent(QObject *sender, MongoShell *shell, const ScriptInfo &scriptInfo) :
            Event(sender),
            shell(shell),
            scriptInfo(scriptInfo) {}

        MongoShell *shell;
        ScriptInfo scriptInfo;
    };

    class ConnectionFailedEvent : public Event
    {
        R_EVENT

        ConnectionFailedEvent(MongoServer *server) :
            Event((QObject *)server),
            server(server) { }

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
        DocumentListLoadedEvent(QObject *sender, int resultIndex, const MongoQueryInfo &queryInfo, const QString &query, const QList<MongoDocumentPtr> &docs) :
            Event(sender),
            _resultIndex(resultIndex),
            _queryInfo(queryInfo),
            _query(query),
            _documents(docs) { }

        int resultIndex() const { return _resultIndex; }
        MongoQueryInfo queryInfo() const { return _queryInfo; }
        QList<MongoDocumentPtr> documents() const { return _documents; }
        QString query() const { return _query; }

    private:
        int _resultIndex;
        MongoQueryInfo _queryInfo;
        QList<MongoDocumentPtr> _documents;
        QString _query;
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
}
