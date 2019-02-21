#pragma once
#include <mongo/bson/bsonobj.h>
#include <mongo/bson/bsonelement.h>

#include "robomongo/core/utils/BsonUtils.h"

namespace Robomongo
{
    class MongoUser
    {
    public:
        typedef std::vector<std::string> RoleType;

        // Creates user from "system.users" document
        explicit MongoUser(const float version, const mongo::BSONObj &obj) :
            _version(version), _readOnly(false)
        {
            // _id = obj.getField("_id"); // todo
            _name = BsonUtils::getField<mongo::String>(obj, "user");
        }

        // Creates new user with empty attributes
        MongoUser(const float version) :
             _version(version), _readOnly(false), _role() {}

        std::string id() const { return _id; }
        std::string name() const { return _name; }
        std::string password() const { return _password; }
        RoleType role() const { return _role; }

        void setName(const std::string &name) { _name = name; }
        void setPassword(const std::string &pwd) { _password = pwd; }
        void setRole(const RoleType &role) { _role = role; }

        std::string userSource() const { return _userSource; }
        void setUserSource(const std::string &source) { _userSource = source; }
        bool readOnly() const { return _readOnly; }
        void setReadOnly(bool readonly) { _readOnly = readonly; }

        float version() const { return _version; }
        static constexpr float minimumSupportedVersion = 2.4f;
    private:
        float _version;
        std::string _name;
        bool _readOnly;
        RoleType _role;
        std::string _password;
        std::string _id;
        std::string _userSource;
    };
}