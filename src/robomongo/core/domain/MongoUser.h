#pragma once
#include <mongo/bson/bsonobj.h>
#include <mongo/bson/bsonelement.h>

#include "robomongo/core/utils/BsonUtils.h"

namespace Robomongo
{
    class MongoUser
    {
    public:
        typedef std::vector<std::string> RolesVector;

        explicit MongoUser(const float version, const mongo::BSONObj &obj) :
            _version(version), _readOnly(false),
            _name(BsonUtils::getField<mongo::String>(obj, "user"))
        {};

        // Creates new user with empty attributes
        MongoUser(const float version) :
             _version(version), _readOnly(false), _roles() {}

        std::string name() const { return _name; }
        std::string password() const { return _password; }
        std::vector<std::string> roles() const { return _roles; }

        void setName(const std::string &name) { _name = name; }
        void setPassword(const std::string &pwd) { _password = pwd; }
        void setRoles(const std::vector<std::string> &roles) { _roles = roles; }

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
        RolesVector _roles;
        std::string _password;
        std::string _userSource;
    };
}