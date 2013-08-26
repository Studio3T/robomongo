#pragma once

#include <QStringList>
#include <mongo/bson/bsonobj.h>
#include "robomongo/core/Core.h"

namespace Robomongo
{
    class Concatenator;

    /*
    ** Represents MongoDB object.
    */
    class MongoDocument
    {
        /*
        ** Owned BSONObj
        */
        const mongo::BSONObj _bsonObj;
    public:
        /*
        ** Constructs empty Document, i.e. { }
        */
        MongoDocument();
        ~MongoDocument();

        /*
        ** Create MongoDocument from BsonObj. It will take owned version of BSONObj
        */
        MongoDocument(mongo::BSONObj bsonObj);

        /*
        ** Create MongoDocument from BsonObj. It will take owned version of BSONObj
        */ 
        static MongoDocumentPtr fromBsonObj(const mongo::BSONObj &bsonObj);

        /*
        ** Create list of MongoDocuments from QList<BsonObj>. It will take owned version of BSONObj
        */ 
        static std::vector<MongoDocumentPtr> fromBsonObj(const std::vector<mongo::BSONObj> &bsonObj);

        /*
        ** Return "native" BSONObj
        */
        mongo::BSONObj bsonObj() const { return _bsonObj; }

        /*
        ** Convert to json string
        */
        void buildJsonString(Concatenator &con);

        /*
        ** Build JsonString from list of documents
        */
        static std::string buildJsonString(const QList<MongoDocumentPtr> &documents);

        static std::string buildJsonString(const MongoDocumentPtr &documents);
    };

    class Concatenator
    {
    public:
        Concatenator();
        void append(const std::string &data);
        std::string build() const;

    private:
        std::vector<std::string> _list;
        int _count;
    };
}
