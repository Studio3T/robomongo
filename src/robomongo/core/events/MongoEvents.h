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
#include "robomongo/core/mongodb/ReplicaSet.h"

namespace Robomongo
{
    class MongoServer;
    class MongoShell;
    class MongoDatabase;
    class MongoWorker;
    class ConnectionSettings;
    class SshTunnelWorker;

    /**
     * @brief EstablishConnection
     */

    class EstablishConnectionRequest : public Event
    {
        R_EVENT

        EstablishConnectionRequest(QObject *sender, ConnectionType connectionType) :
            Event(sender),
            connectionType(connectionType) {}

        ConnectionType connectionType;
    };

    class EstablishConnectionResponse : public Event
    {
        R_EVENT

        enum ErrorReason {
            MongoConnection     = 0,
            MongoAuth           = 1,
            MongoSslConnection  = 2
        };

        EstablishConnectionResponse(QObject *sender, const ConnectionInfo &info, ConnectionType connectionType, 
                                    const ReplicaSet replicaSet) :
            Event(sender),
            _info(info),
            _connectionType(connectionType),
            _replicaSet(replicaSet)
            {}

        EstablishConnectionResponse(QObject *sender, const EventError &error, ConnectionType connectionType, 
                                    const ReplicaSet replicaSet, ErrorReason errorReason) :
            Event(sender, error),
            _info(),
            _connectionType(connectionType),
            _errorReason(errorReason),
            _replicaSet(replicaSet)
        {}

        // Getters - todo: refactor return copy
        ConnectionInfo info() const { return _info; }
        ReplicaSet replicaSet() const { return _replicaSet; }

        const ConnectionInfo _info;
        ConnectionType _connectionType;
        ErrorReason _errorReason;
        const ReplicaSet _replicaSet;
    };

    struct RefreshReplicaSetRequest : public Event
    {
        R_EVENT

        RefreshReplicaSetRequest(QObject *sender) :
            Event(sender) {}
    };

    struct RefreshReplicaSetResponse : public Event
    {
        R_EVENT

        // Primary is reachable
        RefreshReplicaSetResponse(QObject *sender, ReplicaSet replicaSet) :
            Event(sender), replicaSet(replicaSet) {}

        // Primary is unreachable and secondary(ies) might be reachable
        RefreshReplicaSetResponse(QObject *sender, ReplicaSet replicaSet, const EventError &error) :
            Event(sender, error), replicaSet(replicaSet) {}

        ReplicaSet const replicaSet;
    };

    struct ReplicaSetRefreshed : public Event
    {
        R_EVENT

        ReplicaSetRefreshed(QObject *sender) :
            Event(sender) {}

        ReplicaSetRefreshed(QObject *sender, const EventError &error, ReplicaSet replicaSet) :
            Event(sender, error), replicaSet(replicaSet) {}

        ReplicaSet const replicaSet;
    };

    struct RefreshReplicaSetFolderRequest : public Event
    {
        R_EVENT

        RefreshReplicaSetFolderRequest(QObject *sender) :
            Event(sender) {}
    };

    struct RefreshReplicaSetFolderResponse : public Event
    {
        R_EVENT

        // Primary is reachable
        RefreshReplicaSetFolderResponse(QObject *sender, ReplicaSet replicaSet) :
            Event(sender), replicaSet(replicaSet) {}
        
        // Primary is unreachable, secondary(ies) might be reachable
        RefreshReplicaSetFolderResponse(QObject *sender, ReplicaSet replicaSet, const EventError &error) :
            Event(sender, error), replicaSet(replicaSet) {}

        ReplicaSet const replicaSet;
    };

    struct ReplicaSetFolderLoading : public Event
    {
        R_EVENT

            ReplicaSetFolderLoading(QObject *sender) :
            Event(sender) {}
    };

    struct ReplicaSetFolderRefreshed : public Event
    {
        R_EVENT

        ReplicaSetFolderRefreshed(QObject *sender) :
            Event(sender) {}

        ReplicaSetFolderRefreshed(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        ReplicaSetFolderRefreshed(QObject *sender, const EventError &error, ReplicaSet replicaSet) :
            Event(sender, error), replicaSet(replicaSet) {}

        ReplicaSet const replicaSet;
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

        LoadCollectionNamesResponse(QObject *sender, const EventError &error) :
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
        Event(sender), _collection(collection) {}
        MongoCollectionInfo collection() const { return _collection; }
    private:
        const MongoCollectionInfo _collection;
    };

    class LoadCollectionIndexesResponse : public Event
    {
        R_EVENT
    public:
        LoadCollectionIndexesResponse(QObject *sender, const std::vector<EnsureIndexInfo> &indexes) :
            Event(sender), _indexes(indexes) {}

        LoadCollectionIndexesResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}
        std::vector<EnsureIndexInfo> indexes() const { return _indexes; }
    private:
        std::vector<EnsureIndexInfo> _indexes;
    };

    class EnsureIndexRequest : public Event
    {
        R_EVENT
        EnsureIndexRequest(QObject *sender, const EnsureIndexInfo &oldInfo, const EnsureIndexInfo &newInfo) : Robomongo::Event(sender), oldInfo_(oldInfo), newInfo_(newInfo) {}
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
        std::string name() const { return _name; }
    private:
        const MongoCollectionInfo _collection;
        std::string _name;
    };

    class DeleteCollectionIndexResponse: public Event
    {
        R_EVENT
    public:
        DeleteCollectionIndexResponse(QObject *sender, const MongoCollectionInfo &collection, const std::string &index) :
            Event(sender), _collection(collection), _index(index) {}

        DeleteCollectionIndexResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        MongoCollectionInfo collection() const { return _collection; }
        std::string index() const { return _index; }
    private:
        MongoCollectionInfo _collection;
        std::string _index;
    };

    class EditIndexRequest : public Event
    {
        R_EVENT
    public:
        EditIndexRequest(QObject *sender, const MongoCollectionInfo &collection, const std::string &oldIndex, const std::string &newIndex) :
            Event(sender), _collection(collection), _oldIndex(oldIndex), _newIndex(newIndex) {}
        MongoCollectionInfo collection() const { return _collection; }
        std::string oldIndex() const { return _oldIndex; }
        std::string newIndex() const { return _newIndex; }
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

        LoadUsersResponse(QObject *sender, const EventError &error) :
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

        LoadFunctionsResponse(QObject *sender, const EventError &error) :
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
        CreateCollectionRequest(QObject *sender, const MongoNamespace &ns, 
                                                 const mongo::BSONObj& extraOptions,
                                                 long long size = 0,
                                                 bool capped = false,
                                                 int maxDocNum = 0
                                                 ) :
            Event(sender), _ns(ns), _extraOptions(extraOptions),
            _size(size), _capped(capped), _maxDocNum(maxDocNum) {}

        MongoNamespace ns() const { return _ns; }
        long long getSize() const { return _size; }
        bool getCapped() const { return _capped; }
        int getMaxDocNum() const { return _maxDocNum; }
        const mongo::BSONObj getExtraOptions() const { return _extraOptions; }

    private:
        const MongoNamespace _ns;
        const long long _size;
        const bool _capped;
        const int _maxDocNum;
        const mongo::BSONObj _extraOptions;
    };

    struct CreateCollectionResponse : public Event
    {
        R_EVENT

        CreateCollectionResponse(QObject *sender, std::string const& collection) :
        Event(sender), collection(collection) {}

        CreateCollectionResponse(QObject *sender, std::string const& collection, const EventError &error) :
            Event(sender, error), collection(collection) {}

        std::string const collection;
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

    struct DropCollectionResponse : public Event
    {
        R_EVENT

    public:
        DropCollectionResponse(QObject *sender, std::string const& collection) :
            Event(sender), collection(collection) {}

        DropCollectionResponse(QObject *sender, std::string const& collection, const EventError &error) :
            Event(sender, error), collection(collection) {}

        std::string const collection;
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

    struct RenameCollectionResponse : public Event
    {
        R_EVENT

            RenameCollectionResponse(QObject *sender, std::string const& oldCollection, 
                                     std::string const& newCollection) :
            Event(sender), oldCollection(oldCollection), newCollection(newCollection) {}

        RenameCollectionResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

        std::string const oldCollection;
        std::string const newCollection;
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

    struct DuplicateCollectionResponse : public Event
    {
        R_EVENT

    public:
        DuplicateCollectionResponse(QObject *sender, std::string const& sourceCollection,
                                    std::string const& duplicateCollection) :
            Event(sender), sourceCollection(sourceCollection), duplicateCollection(duplicateCollection) {}

        DuplicateCollectionResponse(QObject *sender, std::string const& sourceCollection, const EventError &error) :
            Event(sender, error), sourceCollection(sourceCollection) {}

        std::string const sourceCollection;
        std::string const duplicateCollection;
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
        MongoNamespace from() const { return _from; }
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

        AutocompleteResponse(QObject *sender, const QStringList &list, const std::string &prefix) :
            Event(sender),
            list(list),
            prefix(prefix) {}

        AutocompleteResponse(QObject *sender, const EventError &error) :
            Event(sender, error) {}

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

        ConnectingEvent(QObject *sender) :
            Event(sender) { }
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

        enum Reason {
            MongoConnection    = 1,
            MongoAuth          = 2,
            SshConnection      = 3,
            SshChannel         = 4,
            SslConnection      = 5
        };

        ConnectionFailedEvent(QObject *sender, int serverHandle, ConnectionType connectionType,
                              const std::string& message, Reason reason) :
            Event(sender),
            message(message),
            reason(reason),
            serverHandle(serverHandle),
            connectionType(connectionType) {}

        std::string message;
        Reason reason;
        ConnectionType connectionType;
        int serverHandle;
    };

//    class ScriptExecute

    class ConnectionEstablishedEvent : public Event
    {
        R_EVENT

        ConnectionEstablishedEvent(MongoServer *server, ConnectionType type, ConnectionInfo connInfo) :
            Event((QObject *)server),
            server(server),
            connectionType(type),
            connInfo(connInfo) { }       

    public: // todo
        MongoServer *server;
        ConnectionType connectionType;
        ConnectionInfo connInfo;
    };

    class DatabaseListLoadedEvent : public Event
    {
        R_EVENT

        DatabaseListLoadedEvent(QObject *sender, const QList<MongoDatabase *> &list) :
            Event(sender),
            list(list) { }

        DatabaseListLoadedEvent(QObject *sender, const EventError &error) :
            Event(sender, error) {}

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

        DocumentListLoadedEvent(QObject *sender, const EventError &error) :
            Event(sender, error) {}

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

        ScriptExecutedEvent(QObject *sender, const EventError &error) :
            Event(sender, error) {}

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

    class OperationFailedEvent : public Event
    {
        R_EVENT

    public:
        OperationFailedEvent(QObject *sender, const std::string &technicalErrorMessage, const std::string &userFriendlyErrorMessage) :
            technicalErrorMessage(technicalErrorMessage),
            userFriendlyErrorMessage(userFriendlyErrorMessage),
            Event(sender) { }

        std::string technicalErrorMessage;
        std::string userFriendlyErrorMessage;
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

    /**
     * @brief Establish SSH Connection
     */

    class EstablishSshConnectionRequest : public Event
    {
    R_EVENT

        EstablishSshConnectionRequest(QObject *sender, int serverHandle, SshTunnelWorker* worker, ConnectionSettings* settings, ConnectionType connectionType) :
            Event(sender),
            worker(worker),
            settings(settings),
            serverHandle(serverHandle),
            connectionType(connectionType) {}

        ConnectionSettings* settings;
        ConnectionType connectionType;
        SshTunnelWorker* worker;
        int serverHandle;
    };

    class EstablishSshConnectionResponse : public Event
    {
    R_EVENT

        EstablishSshConnectionResponse(QObject *sender, int serverHandle, SshTunnelWorker* worker, ConnectionSettings* settings, ConnectionType connectionType, int localport) :
            Event(sender),
            worker(worker),
            settings(settings),
            connectionType(connectionType),
            serverHandle(serverHandle),
            localport(localport) {}

        EstablishSshConnectionResponse(QObject *sender, int serverHandle, const EventError &error, SshTunnelWorker* worker, ConnectionSettings* settings, ConnectionType connectionType) :
            Event(sender, error),
            worker(worker),
            settings(settings),
            serverHandle(serverHandle),
            connectionType(connectionType) {}

        ConnectionSettings* settings;
        ConnectionType connectionType;
        SshTunnelWorker* worker;
        int localport;
        int serverHandle;
    };

    /**
     * @brief Start listening on all opened sockets (SSH tunnel)
     */

    class ListenSshConnectionRequest : public Event
    {
    R_EVENT

        ListenSshConnectionRequest(QObject *sender, int serverHandle, ConnectionType connectionType) :
            Event(sender),
            serverHandle(serverHandle),
            connectionType(connectionType) {}

        ConnectionType connectionType;
        int serverHandle;
    };

    class ListenSshConnectionResponse : public Event
    {
    R_EVENT

        ListenSshConnectionResponse(QObject *sender, int serverHandle, ConnectionSettings* settings, ConnectionType connectionType) :
            Event(sender),
            settings(settings),
            serverHandle(serverHandle),
            connectionType(connectionType) {}

        ListenSshConnectionResponse(QObject *sender, const EventError &error, int serverHandle, ConnectionSettings* settings, ConnectionType connectionType) :
            Event(sender, error),
            settings(settings),
            serverHandle(serverHandle),
            connectionType(connectionType) {}

        ConnectionSettings* settings;
        ConnectionType connectionType;
        int serverHandle;
    };

    class LogEvent : public Event
    {
    R_EVENT

        enum LogLevel {
            RBM_ERROR  = 1,
            RBM_WARN   = 2,
            RBM_INFO   = 3,
            RBM_DEBUG  = 100 // log as much as possible
        };

        LogEvent(QObject *sender, const std::string& message, LogLevel level) :
            Event(sender),
            message(message),
            level(level) {}

        std::string message;
        LogLevel level;
    };

    class StopScriptRequest : public Event
    {
    R_EVENT

        StopScriptRequest(QObject *sender) :
            Event(sender) {}
    };
}
