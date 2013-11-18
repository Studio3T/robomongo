#pragma once

#include <vector>

#include "robomongo/core/Core.h"
#include "robomongo/core/EnsureIndex.h"
#include "robomongo/core/utils/ErrorInfo.hpp"
#include "robomongo/core/domain/MongoNamespace.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include <mongo/bson/bsonobj.h>
#include <mongo/client/dbclientinterface.h>


namespace Robomongo
{   
    class MongoServer;

    namespace EventsInfo
    {
        struct EventInfoBase
        {
            EventInfoBase(ErrorInfo er)
                : _errorInfo(er){}

            ErrorInfo errorInfo() const
            {
                return _errorInfo;
            }

            ErrorInfo _errorInfo;
        };

        struct RemoveDocumenInfo 
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            RemoveDocumenInfo(const mongo::Query &query, const MongoNamespace &ns, bool justOne, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er), _query(query), _ns(ns), _justOne(justOne){}

            mongo::Query _query;
            const MongoNamespace _ns;
            bool _justOne;
        };

        struct SaveDocumentInfo 
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            SaveDocumentInfo(const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er), _obj(obj), _ns(ns), _overwrite(overwrite){}

            mongo::BSONObj _obj;
            MongoNamespace _ns;
            bool _overwrite;
        };

        struct DropFunctionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropFunctionInfo(const std::string &database, const std::string &name, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_database(database),_name(name){}

            std::string _database;
            std::string _name;
        };

        struct CreateFunctionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateFunctionInfo(const std::string &database, const MongoFunction &function, const std::string &existingFunctionName = std::string(),
                bool overwrite = false, const ErrorInfo &er = ErrorInfo()) 
                : BaseClass(er),_database(database),_existingFunctionName(existingFunctionName),
                _function(function),_overwrite(overwrite) {}

            std::string _database;
            std::string _existingFunctionName;
            MongoFunction _function;
            bool _overwrite;
        };

        struct CreateUserInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateUserInfo(const std::string &database, const MongoUser &user, bool overwrite, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_database(database), _user(user), _overwrite(overwrite){}

            std::string _database;
            MongoUser _user;
            bool _overwrite;
        };

        struct DropUserInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropUserInfo(const std::string &database, const mongo::OID &id, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_database(database),_id(id)
            {
            }

            std::string _database;
            mongo::OID _id;
        };

        struct CreateCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateCollectionInfo(const MongoNamespace &ns, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_ns(ns){}

            const MongoNamespace _ns;
        };

        struct DropCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropCollectionInfo(const MongoNamespace &ns, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_ns(ns){}

            const MongoNamespace _ns;
        };

        struct RenameCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            RenameCollectionInfo(const MongoNamespace &ns, const std::string &name, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_ns(ns),_name(name){}

            MongoNamespace _ns;
            std::string _name;
        };

        struct DuplicateCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DuplicateCollectionInfo(const MongoNamespace &ns, const std::string &name, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_ns(ns),_name(name){}

            MongoNamespace _ns;
            std::string _name;
        };

        struct CopyCollectionToDiffServerInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CopyCollectionToDiffServerInfo(MongoServer *server, const MongoNamespace &from, const MongoNamespace &to, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_server(server),_from(from),_to(to){}
            MongoServer *_server;
            const MongoNamespace _from;
            const MongoNamespace _to;
        };

        struct CreateIndexInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateIndexInfo(const EnsureIndex &oldIndex, const EnsureIndex &newIndex, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_oldIndex(oldIndex), _newIndex(newIndex){}
            const EnsureIndex _oldIndex;
            const EnsureIndex _newIndex;
        };

        struct DropIndexInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropIndexInfo(const MongoCollectionInfo &collection, const std::string &name, const ErrorInfo &er = ErrorInfo()) 
                : BaseClass(er),_collection(collection), _name(name) {}
            const MongoCollectionInfo _collection;
            std::string _name;
        };

        struct CreateDataBaseInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateDataBaseInfo(const std::string &database, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_database(database){}
            std::string _database;
        };

        struct DropDatabaseInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropDatabaseInfo(const std::string &database, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_database(database){}
            std::string _database;
        };

//////////////////////////////////////////////////////////////////////////
        
        struct ExecuteQueryRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            ExecuteQueryRequestInfo(int resultIndex, const MongoQueryInfo &queryInfo, const ErrorInfo &er = ErrorInfo()) 
                : BaseClass(er),_resultIndex(resultIndex), _queryInfo(queryInfo){}

            const int _resultIndex;
            const MongoQueryInfo _queryInfo;   
        };

        struct ExecuteQueryResponceInfo
            : public ExecuteQueryRequestInfo
        {
            typedef ExecuteQueryRequestInfo BaseClass;
            explicit ExecuteQueryResponceInfo(const ExecuteQueryRequestInfo& req)
                : BaseClass(req),_documents(){}

            ExecuteQueryResponceInfo(int resultIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents, const ErrorInfo &er = ErrorInfo()) 
                : BaseClass(resultIndex, queryInfo, er), _documents(documents){}

            std::vector<MongoDocumentPtr> _documents;   
        };

        struct ExecuteScriptRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            ExecuteScriptRequestInfo(const std::string &script, const std::string &dbName, int take, int skip, const ErrorInfo &er = ErrorInfo()) 
                : BaseClass(er),_script(script),
                _databaseName(dbName), _take(take),
                _skip(skip) {}

            const std::string _script;
            const std::string _databaseName;
            const int _take; //
            const int _skip;
        };

        struct ExecuteScriptResponceInfo
            : public ExecuteScriptRequestInfo
        {
            typedef ExecuteScriptRequestInfo BaseClass;
            explicit ExecuteScriptResponceInfo(const ExecuteScriptRequestInfo& req)
                : BaseClass(req),_result(),_empty() {}

            ExecuteScriptResponceInfo(const std::string &script, const std::string &dbName, int take, int skip, const MongoShellExecResult &result, bool empty, const ErrorInfo &er = ErrorInfo()) 
                : BaseClass(script, dbName, take, skip, er), _result(result), _empty(empty) {}

            MongoShellExecResult _result;
            bool _empty;
        };

        struct EstablishConnectionRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            EstablishConnectionRequestInfo(const ErrorInfo &er = ErrorInfo())
                : BaseClass(er){}
        };

        struct EstablishConnectionResponceInfo
            : public EstablishConnectionRequestInfo
        {
            typedef EstablishConnectionRequestInfo BaseClass;
            explicit EstablishConnectionResponceInfo(const EstablishConnectionRequestInfo& req)
                : BaseClass(req),_info(){}

            EstablishConnectionResponceInfo(const ConnectionInfo &info, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_info(info){}

            ConnectionInfo _info;
        };

        struct LoadFunctionRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadFunctionRequestInfo(const std::string &database, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_database(database){}

            std::string _database;
        };

        struct LoadFunctionResponceInfo
            : public LoadFunctionRequestInfo
        {
            typedef LoadFunctionRequestInfo BaseClass;
            explicit LoadFunctionResponceInfo(const LoadFunctionRequestInfo& req)
                : BaseClass(req),_functions(){}

            LoadFunctionResponceInfo(const std::string &database, const std::vector<MongoFunction> &func, const ErrorInfo &er = ErrorInfo())
                : BaseClass(database, er), _functions(func){}

            std::vector<MongoFunction> _functions;
        };

        struct LoadUserRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadUserRequestInfo(const std::string &database, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_database(database)
            {
            }

            const std::string _database;
        };

        struct LoadUserResponceInfo
            : public LoadUserRequestInfo
        {
            typedef LoadUserRequestInfo BaseClass;
            explicit LoadUserResponceInfo(const LoadUserRequestInfo& req)
                : BaseClass(req),_users(){}

            LoadUserResponceInfo(const std::string &database, const std::vector<MongoUser> &users, const ErrorInfo &er = ErrorInfo())
                : BaseClass(database, er),_users(users)
            {
            }

            std::vector<MongoUser> _users;
        };

        struct LoadCollectionRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadCollectionRequestInfo(const std::string &database, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_database(database)
            {
            }

            const std::string _database;
        };

        struct LoadCollectionResponceInfo
            : public LoadCollectionRequestInfo
        {
            typedef LoadCollectionRequestInfo BaseClass;

            explicit LoadCollectionResponceInfo(const LoadCollectionRequestInfo& req)
                : BaseClass(req),_infos(){}

            LoadCollectionResponceInfo(const std::string &database, const std::vector<MongoCollectionInfo> &infos, const ErrorInfo &er = ErrorInfo())
                : BaseClass(database, er),_infos(infos)
            {
            }

            std::vector<MongoCollectionInfo> _infos;
        };

        struct LoadCollectionIndexesRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadCollectionIndexesRequestInfo(const MongoCollectionInfo &collection, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_collection(collection){}

            const MongoCollectionInfo _collection;
        };

        struct LoadCollectionIndexesResponceInfo
            : public LoadCollectionIndexesRequestInfo
        {
            typedef LoadCollectionIndexesRequestInfo BaseClass;
            explicit LoadCollectionIndexesResponceInfo(const LoadCollectionIndexesRequestInfo& req)
                : BaseClass(req),_indexes(){}

            LoadCollectionIndexesResponceInfo(const MongoCollectionInfo &collection, const std::vector<EnsureIndex> &indexes, const ErrorInfo &er = ErrorInfo())
                : BaseClass(collection,er),_indexes(indexes){}

            std::vector<EnsureIndex> _indexes;
        };

        struct LoadDatabaseNamesRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadDatabaseNamesRequestInfo(const ErrorInfo &er = ErrorInfo())
                : BaseClass(er){}
        };

        struct LoadDatabaseNamesResponceInfo
            : public LoadDatabaseNamesRequestInfo
        {
            typedef LoadDatabaseNamesRequestInfo BaseClass;
            explicit LoadDatabaseNamesResponceInfo(const LoadDatabaseNamesRequestInfo &req):BaseClass(req), _databases(){}

            LoadDatabaseNamesResponceInfo(const std::vector<std::string> &databases, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_databases(databases){}

            std::vector<std::string> _databases;
        };

        struct AutoCompleteRequestInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            AutoCompleteRequestInfo(const std::string &prefix, const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_prefix(prefix){}

            const std::string _prefix;  
        };

        struct AutoCompleteResponceInfo
            : public AutoCompleteRequestInfo
        {
            typedef AutoCompleteRequestInfo BaseClass;
            explicit AutoCompleteResponceInfo(const AutoCompleteRequestInfo& req)
                : BaseClass(req),_list(){}

            AutoCompleteResponceInfo(const std::string &prefix, const QStringList &list, const ErrorInfo &er = ErrorInfo())
                : BaseClass(prefix, er), _list(list){}

            QStringList _list;   //replace QStringList to std::vector    
        };
    }
}
