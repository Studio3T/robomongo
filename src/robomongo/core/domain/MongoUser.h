#pragma once
#include <mongo/bson/bsonobj.h>
#include <mongo/bson/bsonelement.h>

namespace Robomongo
{
    class MongoUser
    {
    public:
        typedef std::vector<std::string> RoleType;
        /**
         * @brief Creates user from "system.users" document
         */
        explicit MongoUser(const float version, const mongo::BSONObj &obj);

        /**
         * @brief Creates new user with empty attributes
         */
        MongoUser(const float version);

        mongo::OID id() const { return _id; }
        std::string name() const { return _name; }
        RoleType role() const { return _role; }
        std::string passwordHash() const { return _passwordHash; }

        void setName(const std::string &name) { _name = name; }
        void setRole(const RoleType &role) { _role = role; }
        void setPasswordHash(const std::string &hash) { _passwordHash = hash; }

        std::string userSource() const { return _userSource; }
        void setUserSource(const std::string &source) { _userSource = source; }
        mongo::BSONObj toBson() const;
        bool readOnly() const { return _readOnly; }
        void setReadOnly(bool readonly) { _readOnly = readonly; }

        float version() const { return _version; }
        static const float minimumSupportedVersion;
    private:
        float _version;
        mongo::OID _id;
        std::string _name;
        bool _readOnly;
        RoleType _role;
        std::string _passwordHash;
        std::string _userSource;
    };
}
