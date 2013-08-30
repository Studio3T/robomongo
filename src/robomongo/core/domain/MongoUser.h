#pragma once
#include <QString>
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    class MongoUser
    {
    public:

        /**
         * @brief Creates user from "system.users" document
         */
        explicit MongoUser(const mongo::BSONObj &obj);

        /**
         * @brief Creates new user with empty attributes
         */
        MongoUser() : _readOnly(false) {}

        mongo::OID id() const { return _id; }
        std::string name() const { return _name; }
        bool readOnly() const { return _readOnly; }
        std::string passwordHash() const { return _passwordHash; }

        void setName(const std::string &name) { _name = name; }
        void setReadOnly(bool readonly) { _readOnly = readonly; }
        void setPasswordHash(const std::string &hash) { _passwordHash = hash; }

        mongo::BSONObj toBson() const;

    private:
        mongo::OID _id;
        std::string _name;
        bool _readOnly;
        std::string _passwordHash;
    };
}
