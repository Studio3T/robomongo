#pragma once

#include <mongo/bson/bsonobj.h>

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

        std::string name() const { return _name; }
        std::string code() const { return _code; }

        void setCode(const std::string &code) { _code = code; }
        void setName(const std::string &name) { _name = name; }

        mongo::BSONObj toBson() const;

    private:
        std::string _name;
        std::string _code;
    };

}
