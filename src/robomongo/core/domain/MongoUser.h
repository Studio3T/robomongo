#pragma once
#include <QString>
#include <mongo/bson/bsonobj.h>
#include <mongo/bson/bsonelement.h>

namespace Robomongo
{
    class MongoUser
    {
    public:
        typedef std::string roleType;
        /**
         * @brief Creates user from "system.users" document
         */
        explicit MongoUser(const mongo::BSONObj &obj);

        /**
         * @brief Creates new user with empty attributes
         */
        MongoUser();

        mongo::OID id() const { return _id; }
        std::string name() const { return _name; }
        roleType role() const { return _role; }
        std::string passwordHash() const { return _passwordHash; }

        void setName(const std::string &name) { _name = name; }
        void setRole(const roleType &role) { _role = role; }
        void setPasswordHash(const std::string &hash) { _passwordHash = hash; }

        mongo::BSONObj toBson() const;

    private:
        mongo::OID _id;
        std::string _name;
        roleType _role;
        std::string _passwordHash;
    };
}
