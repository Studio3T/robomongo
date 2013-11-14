#pragma once

#include <vector>

#include "robomongo/core/utils/ErrorInfo.hpp"
#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoNamespace.h"
#include "robomongo/core/domain/MongoCollectionInfo.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include <mongo/bson/bsonobj.h>
#include <mongo/client/dbclientinterface.h>

namespace Robomongo
{   
    class MongoServer;

    struct EnsureIndex
    {
        EnsureIndex(
            const MongoCollectionInfo &collection,
            const std::string &name = std::string(),
            const std::string &request = std::string(),
            bool isUnique = false,
            bool isBackGround = false,
            bool isDropDuplicates = false,
            bool isSparce = false,
            int expireAfter = 0,
            const std::string &defaultLanguage = std::string(),
            const std::string &languageOverride = std::string(),
            const std::string &textWeights = std::string()):
        _name(name),
            _collection(collection),
            _request(request),
            _unique(isUnique),
            _backGround(isBackGround),
            _dropDups(isDropDuplicates),
            _sparse(isSparce),
            _ttl(expireAfter),
            _defaultLanguage(defaultLanguage),
            _languageOverride(languageOverride),
            _textWeights(textWeights) {}

        MongoCollectionInfo _collection;
        std::string _name;
        std::string _request;
        bool _unique;
        bool _backGround;
        bool _dropDups;
        bool _sparse;
        int _ttl;
        std::string _defaultLanguage;
        std::string _languageOverride;
        std::string _textWeights;
    };

    struct ConnectionInfo
    {
        ConnectionInfo():
            _address(),
            _databases(),
            _version(0.0f) {}
        ConnectionInfo(const std::string &address, const std::vector<std::string> &databases, float version):
            _address(address),
            _databases(databases),
            _version(version) {}

        std::string _address;
        std::vector<std::string> _databases;
        float _version;
    };

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

        struct RemoveDocumentInfo 
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            RemoveDocumentInfo(const mongo::Query &query, const MongoNamespace &ns, bool justOne, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er), _query(query), _ns(ns), _justOne(justOne){}

            mongo::Query _query;
            const MongoNamespace _ns;
            bool _justOne;
        };

        struct SaveDocumentInfo 
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            SaveDocumentInfo(const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er), _obj(obj), _ns(ns), _overwrite(overwrite){}

            mongo::BSONObj _obj;
            MongoNamespace _ns;
            bool _overwrite;
        };

        struct DropFunctionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropFunctionInfo(const std::string &database, const std::string &name, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_database(database),_name(name){}

            std::string _database;
            std::string _name;
        };

        struct CreateFunctionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateFunctionInfo(const std::string &database, const MongoFunction &function, const std::string &existingFunctionName = std::string(),
                bool overwrite = false, const ErrorInfo &er = ErrorInfo()) 
                :BaseClass(er),_database(database),_existingFunctionName(existingFunctionName),
                _function(function),_overwrite(overwrite) {}

            std::string _database;
            std::string _existingFunctionName;
            MongoFunction _function;
            bool _overwrite;
        };

        struct LoadFunctionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadFunctionInfo(const std::string &database, const std::vector<MongoFunction> &func = std::vector<MongoFunction>(), const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_database(database), _functions(func){}

            std::string _database;
            std::vector<MongoFunction> _functions;
        };

        struct CreateUserInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateUserInfo(const std::string &database, const MongoUser &user, bool overwrite, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_database(database), _user(user), _overwrite(overwrite){}

            std::string _database;
            MongoUser _user;
            bool _overwrite;
        };

        struct DropUserInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropUserInfo(const std::string &database, const mongo::OID &id, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_database(database),_id(id)
            {
            }

            std::string _database;
            mongo::OID _id;
        };

        struct LoadUserInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadUserInfo(const std::string &database, const std::vector<MongoUser> &users = std::vector<MongoUser>(), const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_database(database),_users(users)
            {
            }

            std::string _database;
            std::vector<MongoUser> _users;
        };

        struct CreateCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateCollectionInfo(const MongoNamespace &ns, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_ns(ns){}

            const MongoNamespace _ns;
        };

        struct DropCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropCollectionInfo(const MongoNamespace &ns, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_ns(ns){}

            const MongoNamespace _ns;
        };

        struct RenameCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            RenameCollectionInfo(const MongoNamespace &ns, const std::string &name, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_ns(ns),_name(name){}

            MongoNamespace _ns;
            std::string _name;
        };

        struct LoadCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadCollectionInfo(const std::string &database, const std::vector<MongoCollectionInfo> &infos = std::vector<MongoCollectionInfo>(), const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_database(database),_infos(infos)
            {
            }

            std::string _database;
            std::vector<MongoCollectionInfo> _infos;
        };

        struct DuplicateCollectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DuplicateCollectionInfo(const MongoNamespace &ns, const std::string &name, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_ns(ns),_name(name){}

            MongoNamespace _ns;
            std::string _name;
        };

        struct CopyCollectionToDiffServerInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CopyCollectionToDiffServerInfo(MongoServer *server, const MongoNamespace &from, const MongoNamespace &to, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_server(server),_from(from),_to(to){}
            MongoServer *_server;
            const MongoNamespace _from;
            const MongoNamespace _to;
        };

        struct LoadCollectionIndexesInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadCollectionIndexesInfo(const MongoCollectionInfo &collection,const std::vector<EnsureIndex> &indexes = std::vector<EnsureIndex>(), const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_collection(collection),_indexes(indexes){}
            MongoCollectionInfo _collection;
            std::vector<EnsureIndex> _indexes;
        };

        struct CreateIndexInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateIndexInfo(const EnsureIndex &oldIndex, const EnsureIndex &newIndex, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_oldIndex(oldIndex), _newIndex(newIndex){}
            const EnsureIndex _oldIndex;
            const EnsureIndex _newIndex;
        };

        struct DropIndexInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            DropIndexInfo(const MongoCollectionInfo &collection, const std::string &name, const ErrorInfo &er = ErrorInfo()) 
                :BaseClass(er),_collection(collection), _name(name) {}
            const MongoCollectionInfo _collection;
            std::string _name;
        };

        struct CreateDataBaseInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            CreateDataBaseInfo(const std::string &database, const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_database(database){}
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

        struct LoadDatabaseNamesInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            LoadDatabaseNamesInfo(const std::vector<std::string> &database = std::vector<std::string>(), const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_databaseNames(database){}
            std::vector<std::string> _databaseNames;
        };

        struct AutoCompleteInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            AutoCompleteInfo(const std::string &prefix, const QStringList &list = QStringList(), const ErrorInfo &er = ErrorInfo())
                : BaseClass(er),_prefix(prefix), _list(list){}

            std::string _prefix;
            QStringList _list;   //replace QStringList to std::vector    
        };

        struct ExecuteQueryInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            ExecuteQueryInfo(int resultIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents = std::vector<MongoDocumentPtr>(), const ErrorInfo &er = ErrorInfo()) 
                : BaseClass(er),_resultIndex(resultIndex), _queryInfo(queryInfo), _documents(documents){}

            int _resultIndex;
            MongoQueryInfo _queryInfo;
            std::vector<MongoDocumentPtr> _documents;   
        };

        struct ExecuteScriptInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            ExecuteScriptInfo(const std::string &script, const std::string &dbName, int take, int skip, const MongoShellExecResult &result = MongoShellExecResult(), bool empty = false, const ErrorInfo &er = ErrorInfo()) 
                :BaseClass(er),_script(script),
                _databaseName(dbName), _take(take),
                _skip(skip),_result(result),
                _empty(empty) {}

            std::string _script;
            std::string _databaseName;
            int _take; //
            int _skip;

            MongoShellExecResult _result;
            bool _empty;
        };

        struct EstablishConnectionInfo
            : public EventInfoBase
        {
            typedef EventInfoBase BaseClass;
            EstablishConnectionInfo(const ConnectionInfo &info = ConnectionInfo(), const ErrorInfo &er = ErrorInfo())
                :BaseClass(er),_info(info){}

            ConnectionInfo _info;
        };
    }
}
