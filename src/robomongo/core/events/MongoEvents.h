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
#include "robomongo/core/Enums.h"

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

        EstablishConnectionResponse(QObject *sender, const ConnectionInfo &info) :
            Event(sender),
            _info(info) {}

        EstablishConnectionResponse(QObject *sender, const EventError &error) :
            Event(sender, error),_info() {}

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

        LoadDatabaseNamesResponse(QObject *sender, const std::vector<std::string> &databaseNames) :
            Event(sender),
            databaseNames(databaseNames) {}

        LoadDatabaseNamesResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        std::vector<std::string> databaseNames;
    };


    /**
     * @brief LoadCollectionNames
     */

    class LoadCollectionNamesRequest : public Event
    {
        R_EVENT

    public:
        LoadCollectionNamesRequest(QObject *sender, const std::string &databaseName) :
            Event(sender),
            _databaseName(databaseName) {}

        std::string databaseName() const { return _databaseName; }

    private:
        std::string _databaseName;
    };

    class LoadCollectionNamesResponse : public Event
    {
        R_EVENT

    public:
        LoadCollectionNamesResponse(QObject *sender, const std::string &databaseName,
                                    const std::vector<MongoCollectionInfo> &collectionInfos) :
            Event(sender),
            _databaseName(databaseName),
            _collectionInfos(collectionInfos) { }

        LoadCollectionNamesResponse(QObject *sender,const EventError &error) :
            Event(sender, error) {}

        std::string databaseName() const { return _databaseName; }
        std::vector<MongoCollectionInfo> collectionInfos() const { return _collectionInfos; }

    private:
        std::string _databaseName;
        std::vector<MongoCollectionInfo> _collectionInfos;
    };

    class LoadCollectionIndexesRequest : public Event
    {
        R_EVENT
    public:
        LoadCollectionIndexesRequest(QObject *sender, const MongoCollectionInfo &collection) :
        Event(sender),_collection(collection) {}
        MongoCollectionInfo collection() const { return _collection; }
    private:
        const MongoCollectionInfo _collection;
    };

    class LoadCollectionIndexesResponse : public Event
    {
        R_EVENT
    public:
        LoadCollectionIndexesResponse(QObject *sender, const std::vector<EnsureIndexInfo> &indexes) :Event(sender), _indexes(indexes) {}
        std::vector<EnsureIndexInfo> indexes() const { return _indexes; }
    private:
        std::vector<EnsureIndexInfo> _indexes;
    };

    class EnsureIndexRequest : public Event
    {
        R_EVENT
        EnsureIndexRequest(QObject *sender,const EnsureIndexInfo &oldInfo,const EnsureIndexInfo &newInfo) : Robomongo::Event(sender),oldInfo_(oldInfo),newInfo_(newInfo) {}
        const EnsureIndexInfo &oldInfo() const
        {
            return oldInfo_;
        }
        const EnsureIndexInfo &newInfo() const
        {
            return newInfo_;
        }
    private:
        const EnsureIndexInfo oldInfo_;
        const EnsureIndexInfo newInfo_;
    };

    class DropCollectionIndexRequest : public Event
    {
        R_EVENT
    public:
        DropCollectionIndexRequest(QObject *sender, const MongoCollectionInfo &collection, const std::string &name) :
            Event(sender),
            _collection(collection),
            _name(name) {}

        MongoCollectionInfo collection() const { return _collection; }
        std::string name() const {return _name;}
    private:
        const MongoCollectionInfo _collection;
        std::string _name;
    };

    class DeleteCollectionIndexResponse: public Event
    {
        R_EVENT
    public:
        DeleteCollectionIndexResponse(QObject *sender, const MongoCollectionInfo &collection,const std::string &index) :
        Event(sender),_collection(collection),_index(index) {}
        MongoCollectionInfo collection() const { return _collection; }
        std::string index() const {return _index;}
    private:
        const MongoCollectionInfo _collection;
        std::string _index;
    };

    class EditIndexRequest : public Event
    {
        R_EVENT
    public:
        EditIndexRequest(QObject *sender, const MongoCollectionInfo &collection,const std::string &oldIndex,const std::string &newIndex) :
            Event(sender),_collection(collection),_oldIndex(oldIndex),_newIndex(newIndex) {}
        MongoCollectionInfo collection() const { return _collection; }
        std::string oldIndex() const {return _oldIndex;}
        std::string newIndex() const {return _newIndex;}
    private:
        const MongoCollectionInfo _collection;
        std::string _oldIndex;
        std::string _newIndex;
    };

    /**
     * @brief Load Users
     */

    class LoadUsersRequest : public Event
    {
        R_EVENT

    public:
        LoadUsersRequest(QObject *sender, const std::string &databaseName) :
            Event(sender),
            _databaseName(databaseName) {}

        std::string databaseName() const { return _databaseName; }

    private:
        std::string _databaseName;
    };

    class LoadUsersResponse : public Event
    {
        R_EVENT

    public:
        LoadUsersResponse(QObject *sender, const std::string &databaseName,
                                    const std::vector<MongoUser> &users) :
            Event(sender),
            _databaseName(databaseName),
            _users(users) { }

        LoadUsersResponse(QObject *sender,const EventError &error) :
            Event(sender, error) {}

        std::string databaseName() const { return _databaseName; }
        std::vector<MongoUser> users() const { return _users; }

    private:
        std::string _databaseName;
        std::vector<MongoUser> _users;
    };


    /**
     * @brief Load Functions
     */

    class LoadFunctionsRequest : public Event
    {
        R_EVENT

    public:
        LoadFunctionsRequest(QObject *sender, const std::string &databaseName) :
            Event(sender),
            _databaseName(databaseName) {}

        std::string databaseName() const { return _databaseName; }

    private:
        std::string _databaseName;
    };

    class LoadFunctionsResponse : public Event
    {
        R_EVENT

    public:
        LoadFunctionsResponse(QObject *sender, const std::string &databaseName,
                                    const std::vector<MongoFunction> &functions) :
            Event(sender),
            _databaseName(databaseName),
            _functions(functions) { }

        LoadFunctionsResponse(QObject *sender,const EventError &error) :
            Event(sender, error) {}

        std::string databaseName() const { return _databaseName; }
        std::vector<MongoFunction> functions() const { return _functions; }

    private:
        std::string _databaseName;
        std::vector<MongoFunction> _functions;
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
        InsertDocumentResponse(QObject *sender) :
            Event(sender) {}

        InsertDocumentResponse(QObject *sender, const EventError &error) :
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
        RemoveDocumentResponse(QObject *sender) :
            Event(sender) {}

        RemoveDocumentResponse(QObject *sender, const EventError &error) :
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
        CreateDatabaseResponse(QObject *sender) :
            Event(sender) {}

        CreateDatabaseResponse(QObject *sender, const EventError &error) :
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
        DropDatabaseResponse(QObject *sender) :
            Event(sender) {}

        DropDatabaseResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };

    /**
     * @brief Create Collection
     */

    class CreateCollectionRequest : public Event
    {
        R_EVENT

    public:
        CreateCollectionRequest(QObject *sender, const MongoNamespace &ns) :
            Event(sender),
            _ns(ns) {}

        MongoNamespace ns() const { return _ns; }

    private:
        const MongoNamespace _ns;
    };

    class CreateCollectionResponse : public Event
    {
        R_EVENT

    public:
        CreateCollectionResponse(QObject *sender) :
            Event(sender) {}

        CreateCollectionResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };



    /**
     * @brief Drop Collection
     */

    class DropCollectionRequest : public Event
    {
        R_EVENT

    public:
        DropCollectionRequest(QObject *sender, const MongoNamespace &ns) :
            Event(sender),
            _ns(ns) {}

        MongoNamespace ns() const { return _ns; }

    private:
        MongoNamespace _ns;
    };

    class DropCollectionResponse : public Event
    {
        R_EVENT

    public:
        DropCollectionResponse(QObject *sender) :
            Event(sender) {}

        DropCollectionResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };

    /**
     * @brief Rename Collection
     */

    class RenameCollectionRequest : public Event
    {
        R_EVENT

    public:
        RenameCollectionRequest(QObject *sender, const MongoNamespace &ns, const std::string &newCollection) :
            Event(sender),
            _ns(ns),
            _newCollection(newCollection) {}

        MongoNamespace ns() const { return _ns; }
        std::string newCollection() const { return _newCollection; }

    private:
        MongoNamespace _ns;
        std::string _newCollection;
    };

    class RenameCollectionResponse : public Event
    {
        R_EVENT

    public:
        RenameCollectionResponse(QObject *sender) :
            Event(sender) {}

        RenameCollectionResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };

    /**
     * @brief Clone Collection
     */

    class DuplicateCollectionRequest : public Event
    {
        R_EVENT

    public:
        DuplicateCollectionRequest(QObject *sender, const MongoNamespace &ns, const std::string &newCollection) :
            Event(sender),
            _ns(ns),
            _newCollection(newCollection) {}

        MongoNamespace ns() const { return _ns; }
        std::string newCollection() const { return _newCollection; }

    private:
        MongoNamespace _ns;
        std::string _newCollection;
    };

    class DuplicateCollectionResponse : public Event
    {
        R_EVENT

    public:
        DuplicateCollectionResponse(QObject *sender) :
            Event(sender) {}

        DuplicateCollectionResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };

     /**
     * @brief Copy collection to diffrent server
     */

    class CopyCollectionToDiffServerRequest : public Event
    {
        R_EVENT

    public:
        CopyCollectionToDiffServerRequest(QObject *sender, MongoWorker *worker, const std::string &databaseFrom,
            const std::string &collection, const std::string &databaseTo) :
        Event(sender),
            _worker(worker),
            _from(databaseFrom, collection),
            _to(databaseTo, collection) {}

        MongoWorker *worker() const { return _worker; }
        MongoNamespace from() const { return _from;}
        MongoNamespace to() const { return _to; }
    private:
        MongoWorker *_worker;
        const MongoNamespace _from;
        const MongoNamespace _to;
    };

    class CopyCollectionToDiffServerResponse : public Event
    {
        R_EVENT

    public:
        CopyCollectionToDiffServerResponse(QObject *sender) :
            Event(sender) {}

        CopyCollectionToDiffServerResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };

    /**
     * @brief Create User
     */

    class CreateUserRequest : public Event
    {
        R_EVENT

    public:
        CreateUserRequest(QObject *sender, const std::string &database, const MongoUser &user, bool overwrite = false) :
            Event(sender),
            _database(database),
            _user(user),
            _overwrite(overwrite) {}

        std::string database() const { return _database; }
        MongoUser user() const { return _user; }
        bool overwrite() const { return _overwrite; }

    private:
        std::string _database;
        MongoUser _user;
        bool _overwrite;
    };

    class CreateUserResponse : public Event
    {
        R_EVENT

    public:
        CreateUserResponse(QObject *sender) :
            Event(sender) {}

        CreateUserResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };


    /**
     * @brief Drop User
     */

    class DropUserRequest : public Event
    {
        R_EVENT

    public:
        DropUserRequest(QObject *sender, const std::string &database, const mongo::OID &id) :
            Event(sender),
            _database(database),
            _id(id) {}

        std::string database() const { return _database; }
        mongo::OID id() const { return _id; }

    private:
        std::string _database;
        mongo::OID _id;
    };

    class DropUserResponse : public Event
    {
        R_EVENT

    public:
        DropUserResponse(QObject *sender) :
            Event(sender) {}

        DropUserResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };


    /**
     * @brief Create Function
     */

    class CreateFunctionRequest : public Event
    {
        R_EVENT

    public:
        CreateFunctionRequest(QObject *sender, const std::string &database, const MongoFunction &function,
                              const std::string &existingFunctionName = std::string(), bool overwrite = false) :
            Event(sender),
            _database(database),
            _existingFunctionName(existingFunctionName),
            _function(function),
            _overwrite(overwrite) {}

        std::string database() const { return _database; }
        std::string existingFunctionName() const { return _existingFunctionName; }
        MongoFunction function() const { return _function; }
        bool overwrite() const { return _overwrite; }

    private:
        std::string _database;
        std::string _existingFunctionName;
        MongoFunction _function;
        bool _overwrite;
    };

    class CreateFunctionResponse : public Event
    {
        R_EVENT

    public:
        CreateFunctionResponse(QObject *sender) :
            Event(sender) {}

        CreateFunctionResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
    };


    /**
     * @brief Drop Function
     */

    class DropFunctionRequest : public Event
    {
        R_EVENT

    public:
        DropFunctionRequest(QObject *sender, const std::string &database, const std::string &name) :
            Event(sender),
            _database(database),
            _name(name) {}

        std::string database() const { return _database; }
        std::string name() const { return _name; }

    private:
        std::string _database;
        std::string _name;
    };

    class DropFunctionResponse : public Event
    {
        R_EVENT

    public:
        DropFunctionResponse(QObject *sender) :
            Event(sender) {}

        DropFunctionResponse(QObject *sender, const EventError &error) :
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

        ExecuteQueryResponse(QObject *sender, int resultIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents) :
            Event(sender),
            resultIndex(resultIndex),
            queryInfo(queryInfo),
            documents(documents) { }

        ExecuteQueryResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        int resultIndex;
        MongoQueryInfo queryInfo;
        std::vector<MongoDocumentPtr> documents;
    };

    class AutocompleteRequest : public Event
    {
        R_EVENT

        AutocompleteRequest(QObject *sender, const std::string &prefix, const AutocompletionMode mode) :
            Event(sender),
            prefix(prefix),
            mode(mode) {}

        std::string prefix;
        AutocompletionMode mode;
    };

    class AutocompleteResponse : public Event
    {
        R_EVENT

        AutocompleteResponse(QObject *sender,const QStringList &list, const std::string &prefix) :
            Event(sender),
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

        ExecuteScriptResponse(QObject *sender, const MongoShellExecResult &result, bool empty) :
            Event(sender),
            result(result),
            empty(empty) { }

        ExecuteScriptResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

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

        ConnectionFailedEvent(MongoServer *server, const EventError &error) :
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
