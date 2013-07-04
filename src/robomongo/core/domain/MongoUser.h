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
        MongoUser(const mongo::BSONObj &obj);

        /**
         * @brief Creates new user with empty attributes
         */
        MongoUser() : _readOnly(false) {}

        mongo::OID id() const { return _id; }
        QString name() const { return _name; }
        bool readOnly() const { return _readOnly; }
        QString passwordHash() const { return _passwordHash; }

        void setName(const QString &name) { _name = name; }
        void setReadOnly(bool readonly) { _readOnly = readonly; }
        void setPasswordHash(const QString &hash) { _passwordHash = hash; }

        mongo::BSONObj toBson() const;

    private:
        mongo::OID _id;
        QString _name;
        bool _readOnly;
        QString _passwordHash;
    };
}
