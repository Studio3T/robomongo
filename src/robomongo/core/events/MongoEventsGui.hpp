#pragma once

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/domain/MongoNamespace.h"
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    struct RemoveDocumentInfo 
    {
        RemoveDocumentInfo(const mongo::Query &query, const MongoNamespace &ns, bool justOne)
            :_query(query), _ns(ns), _justOne(justOne){}

        mongo::Query _query;
        const MongoNamespace _ns;
        bool _justOne;
    };

    struct RemoveDocumentEvent 
        : public QtUtils::Event<RemoveDocumentEvent, RemoveDocumentInfo>
    {
        typedef QtUtils::Event<RemoveDocumentEvent, RemoveDocumentInfo> BaseClass;
        enum{ EventType = User+1 };
        RemoveDocumentEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct SaveDocumentInfo 
    {
        SaveDocumentInfo(const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite)
            :_obj(obj), _ns(ns), _overwrite(overwrite){}

        mongo::BSONObj _obj;
        MongoNamespace _ns;
        bool _overwrite;
    };

    struct SaveDocumentEvent 
        : public QtUtils::Event<SaveDocumentEvent, SaveDocumentInfo>
    {
        typedef QtUtils::Event<SaveDocumentEvent, SaveDocumentInfo> BaseClass;
        enum{ EventType = User+2 };
        SaveDocumentEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct DropFunctionInfo
    {
        DropFunctionInfo(const std::string &database, const std::string &name)
            :_database(database), _name(name){}
        std::string _database;
        std::string _name;
    };

    struct DropFunctionEvent
        : public QtUtils::Event<DropFunctionEvent, DropFunctionInfo>
    {
        typedef QtUtils::Event<DropFunctionEvent, DropFunctionInfo> BaseClass;
        enum{ EventType = User+3 };
        DropFunctionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct CreateFunctionInfo
    {
        CreateFunctionInfo(const std::string &database, const MongoFunction &function,
            const std::string &existingFunctionName = std::string(), bool overwrite = false) :
            _database(database),
            _existingFunctionName(existingFunctionName),
            _function(function),
            _overwrite(overwrite) {}

        std::string _database;
        std::string _existingFunctionName;
        MongoFunction _function;
        bool _overwrite;
    };

    struct CreateFunctionEvent
        : public QtUtils::Event<CreateFunctionEvent, CreateFunctionInfo>
    {
        typedef QtUtils::Event<CreateFunctionEvent, CreateFunctionInfo> BaseClass;
        enum{ EventType = User+4 };
        CreateFunctionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct LoadFunctionInfo
    {
        LoadFunctionInfo(const std::string &database, const std::vector<MongoFunction> &func = std::vector<MongoFunction>())
            :_database(database), _functions(func){}
        std::string _database;
        std::vector<MongoFunction> _functions;
    };

    struct LoadFunctionEvent
        : public QtUtils::Event<LoadFunctionEvent, LoadFunctionInfo>
    {
        typedef QtUtils::Event<LoadFunctionEvent, LoadFunctionInfo> BaseClass;
        enum{ EventType = User+5 };
        LoadFunctionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct CreateUserInfo
    {
        CreateUserInfo(const std::string &database, const MongoUser &user, bool overwrite)
            :_database(database), _user(user), _overwrite(overwrite){}
        std::string _database;
        MongoUser _user;
        bool _overwrite;
    };

    struct CreateUserEvent
        : public QtUtils::Event<CreateUserEvent, CreateUserInfo>
    {
        typedef QtUtils::Event<CreateUserEvent, CreateUserInfo> BaseClass;
        enum{ EventType = User+6 };
        CreateUserEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct DropUserInfo
    {
        DropUserInfo(const std::string &database, const mongo::OID &id):
            _database(database),_id(id)
        {
        }
        std::string _database;
        mongo::OID _id;
    };

    struct DropUserEvent
        : public QtUtils::Event<DropUserEvent, DropUserInfo>
    {
        typedef QtUtils::Event<DropUserEvent, DropUserInfo> BaseClass;
        enum{ EventType = User+7 };
        DropUserEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct LoadUserInfo
    {
        LoadUserInfo(const std::string &database, const std::vector<MongoUser> &users = std::vector<MongoUser>()):
            _database(database),_users(users)
        {
        }
        std::string _database;
        std::vector<MongoUser> _users;
    };

    struct LoadUserEvent
        : public QtUtils::Event<LoadUserEvent, LoadUserInfo>
    {
        typedef QtUtils::Event<LoadUserEvent, LoadUserInfo> BaseClass;
        enum{ EventType = User+8 };
        LoadUserEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct CreateCollectionInfo
    {
        CreateCollectionInfo(const MongoNamespace &ns)
            :_ns(ns){}

        const MongoNamespace _ns;
    };

    struct CreateCollectionEvent
        : public QtUtils::Event<CreateCollectionEvent, CreateCollectionInfo>
    {
        typedef QtUtils::Event<CreateCollectionEvent, CreateCollectionInfo> BaseClass;
        enum{ EventType = User+9 };
        CreateCollectionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct DropCollectionInfo
    {
        DropCollectionInfo(const MongoNamespace &ns)
            :_ns(ns){}

        const MongoNamespace _ns;
    };

    struct DropCollectionEvent
        : public QtUtils::Event<DropCollectionEvent, DropCollectionInfo>
    {
        typedef QtUtils::Event<DropCollectionEvent, DropCollectionInfo> BaseClass;
        enum{ EventType = User+10 };
        DropCollectionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct RenameCollectionInfo
    {
        RenameCollectionInfo(const MongoNamespace &ns, const std::string &name)
            :_ns(ns), _name(name){}

        MongoNamespace _ns;
        std::string _name;
    };

    struct RenameCollectionEvent
        : public QtUtils::Event<RenameCollectionEvent, RenameCollectionInfo>
    {
        typedef QtUtils::Event<RenameCollectionEvent, RenameCollectionInfo> BaseClass;
        enum{ EventType = User+11 };
        RenameCollectionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct LoadCollectionInfo
    {
        LoadCollectionInfo(const std::string &database, const std::vector<MongoCollectionInfo> &infos = std::vector<MongoCollectionInfo>()):
            _database(database),_infos(infos)
        {
        }
        std::string _database;
        std::vector<MongoCollectionInfo> _infos;
    };

    struct LoadCollectionEvent
        : public QtUtils::Event<LoadCollectionEvent, LoadCollectionInfo>
    {
        typedef QtUtils::Event<LoadCollectionEvent, LoadCollectionInfo> BaseClass;
        enum{ EventType = User+12 };
        LoadCollectionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct DuplicateCollectionInfo
    {
        DuplicateCollectionInfo(const MongoNamespace &ns, const std::string &name)
            :_ns(ns), _name(name){}

        MongoNamespace _ns;
        std::string _name;
    };

    struct DuplicateCollectionEvent
        : public QtUtils::Event<DuplicateCollectionEvent, DuplicateCollectionInfo>
    {
        typedef QtUtils::Event<DuplicateCollectionEvent, DuplicateCollectionInfo> BaseClass;
        enum{ EventType = User+13 };
        DuplicateCollectionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct CopyCollectionToDiffServerInfo
    {
        CopyCollectionToDiffServerInfo(MongoWorker *worker, const MongoNamespace &from, const MongoNamespace &to)
            :_worker(worker),_from(from),_to(to){}
        MongoWorker *_worker;
        const MongoNamespace _from;
        const MongoNamespace _to;
    };

    struct CopyCollectionToDiffServerEvent
        : public QtUtils::Event<CopyCollectionToDiffServerEvent, CopyCollectionToDiffServerInfo>
    {
        typedef QtUtils::Event<CopyCollectionToDiffServerEvent, CopyCollectionToDiffServerInfo> BaseClass;
        enum{ EventType = User+14 };
        CopyCollectionToDiffServerEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct LoadCollectionIndexesInfo
    {
        LoadCollectionIndexesInfo(const MongoCollectionInfo &collection,const std::vector<EnsureIndex> &indexes = std::vector<EnsureIndex>())
            :_collection(collection),_indexes(indexes){}
        MongoCollectionInfo _collection;
        std::vector<EnsureIndex> _indexes;
    };

    struct LoadCollectionIndexEvent
        : public QtUtils::Event<LoadCollectionIndexEvent, LoadCollectionIndexesInfo>
    {
        typedef QtUtils::Event<LoadCollectionIndexEvent, LoadCollectionIndexesInfo> BaseClass;
        enum{ EventType = User+15 };
        LoadCollectionIndexEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct EnsureIndexInfo
    {
        EnsureIndexInfo(const EnsureIndex &oldIndex, const EnsureIndex &newIndex)
            :_oldIndex(oldIndex), _newIndex(newIndex){}
        const EnsureIndex _oldIndex;
        const EnsureIndex _newIndex;
    };

    struct CreateIndexEvent
        : public QtUtils::Event<CreateIndexEvent, EnsureIndexInfo>
    {
        typedef QtUtils::Event<CreateIndexEvent, EnsureIndexInfo> BaseClass;
        enum{ EventType = User+16 };
        CreateIndexEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct DeleteIndexInfo
    {
        DeleteIndexInfo(const MongoCollectionInfo &collection, const std::string &name) 
            : _collection(collection), _name(name) {}
        const MongoCollectionInfo _collection;
        std::string _name;
    };

    struct DeleteIndexEvent
        : public QtUtils::Event<DeleteIndexEvent, DeleteIndexInfo>
    {
        typedef QtUtils::Event<DeleteIndexEvent, DeleteIndexInfo> BaseClass;
        enum{ EventType = User+17 };
        DeleteIndexEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct CreateDataBaseInfo
    {
        CreateDataBaseInfo(const std::string &database)
            : _database(database){}
        std::string _database;
    };

    struct CreateDataBaseEvent
        : public QtUtils::Event<CreateDataBaseEvent, CreateDataBaseInfo>
    {
        typedef QtUtils::Event<CreateDataBaseEvent, CreateDataBaseInfo> BaseClass;
        enum{ EventType = User+18 };
        CreateDataBaseEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct DropDatabaseInfo
    {
        DropDatabaseInfo(const std::string &database)
            : _database(database){}
        std::string _database;
    };

    struct DropDatabaseEvent
        : public QtUtils::Event<DropDatabaseEvent, DropDatabaseInfo>
    {
        typedef QtUtils::Event<DropDatabaseEvent, DropDatabaseInfo> BaseClass;
        enum{ EventType = User+19 };
        DropDatabaseEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };
    
    struct LoadDatabaseNamesInfo
    {
        LoadDatabaseNamesInfo(const std::vector<std::string> &database = std::vector<std::string>())
            : _databaseNames(database){}
        std::vector<std::string> _databaseNames;
    };

    struct LoadDatabaseNamesEvent
        : public QtUtils::Event<LoadDatabaseNamesEvent, LoadDatabaseNamesInfo>
    {
        typedef QtUtils::Event<LoadDatabaseNamesEvent, LoadDatabaseNamesInfo> BaseClass;
        enum{ EventType = User+20 };
        LoadDatabaseNamesEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct AutoCompleteInfo
    {
        AutoCompleteInfo(const std::string &prefix, const QStringList &list = QStringList())
            : _prefix(prefix), _list(list){}

        std::string _prefix;
        QStringList _list;   //replace QStringList to std::vector    
    };

    struct AutoCompleteEvent
        : public QtUtils::Event<AutoCompleteEvent, AutoCompleteInfo>
    {
        typedef QtUtils::Event<AutoCompleteEvent, AutoCompleteInfo> BaseClass;
        enum{ EventType = User+21 };
        AutoCompleteEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct ExecuteQueryInfo
    {
        ExecuteQueryInfo(int resultIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents = std::vector<MongoDocumentPtr>()) 
            : _resultIndex(resultIndex), _queryInfo(queryInfo), _documents(documents){}

        int _resultIndex;
        MongoQueryInfo _queryInfo;
        std::vector<MongoDocumentPtr> _documents;   
    };

    struct ExecuteQueryEvent
        : public QtUtils::Event<ExecuteQueryEvent, ExecuteQueryInfo>
    {
        typedef QtUtils::Event<ExecuteQueryEvent, ExecuteQueryInfo> BaseClass;
        enum{ EventType = User+22 };
        ExecuteQueryEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };

    struct ExecuteScriptInfo
    {
        ExecuteScriptInfo(const std::string &script, const std::string &dbName, int take, int skip, const MongoShellExecResult &result = MongoShellExecResult(), bool empty = false) :
            _script(script),
            _databaseName(dbName),
            _take(take),
            _skip(skip),
            _result(result),
            _empty(empty) {}

        std::string _script;
        std::string _databaseName;
        int _take; //
        int _skip;

        MongoShellExecResult _result;
        bool _empty;
    };

    struct ExecuteScriptEvent
        : public QtUtils::Event<ExecuteScriptEvent, ExecuteScriptInfo>
    {
        typedef QtUtils::Event<ExecuteScriptEvent, ExecuteScriptInfo> BaseClass;
        enum{ EventType = User+23 };
        ExecuteScriptEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };
    
    struct EstablishConnectionInfo
    {
        EstablishConnectionInfo(const ConnectionInfo &info = ConnectionInfo())
            :_info(info){}
        ConnectionInfo _info;
    };

    struct EstablishConnectionEvent
        : public QtUtils::Event<EstablishConnectionEvent, EstablishConnectionInfo>
    {
        typedef QtUtils::Event<EstablishConnectionEvent, EstablishConnectionInfo> BaseClass;
        enum{ EventType = User+24 };
        EstablishConnectionEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };
}
