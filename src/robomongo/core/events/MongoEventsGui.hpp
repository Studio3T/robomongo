#pragma once

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/domain/MongoNamespace.h"
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    struct SaveObjectInfo 
    {
        SaveObjectInfo(const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite)
            :_obj(obj), _ns(ns), _overwrite(overwrite){}

        mongo::BSONObj _obj;
        MongoNamespace _ns;
        bool _overwrite;
    };

    struct SaveObjectEvent 
        : public QtUtils::Event<SaveObjectEvent, SaveObjectInfo>
    {
        typedef QtUtils::Event<SaveObjectEvent, SaveObjectInfo> BaseClass;
        enum{ EventType = User+1 };
        SaveObjectEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
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
        enum{ EventType = User+2 };
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
        enum{ EventType = User+3 };
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
        enum{ EventType = User+4 };
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
        enum{ EventType = User+5 };
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
        enum{ EventType = User+6 };
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
        enum{ EventType = User+7 };
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
        enum{ EventType = User+8 };
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
        enum{ EventType = User+9 };
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
        enum{ EventType = User+10 };
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
        enum{ EventType = User+11 };
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
        enum{ EventType = User+12 };
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
        enum{ EventType = User+13 };
        CopyCollectionToDiffServerEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };
}
