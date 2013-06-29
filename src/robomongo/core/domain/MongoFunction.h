#pragma once
#include <mongo/client/dbclient.h>
#include <QString>

namespace Robomongo
{
    class MongoFunction
    {
    public:
        /**
         * @brief Creates function from "system.js" document
         */
        MongoFunction(const mongo::BSONObj &obj);

        /**
         * @brief Creates new function with empty attributes
         */
        MongoFunction() {}

        QString name() const { return _name; }
        QString code() const { return _code; }

        void setCode(const QString &code) { _code = code; }
        void setName(const QString &name) { _name = name; }

        mongo::BSONObj toBson() const;

    private:
        QString _name;
        QString _code;
    };

}
