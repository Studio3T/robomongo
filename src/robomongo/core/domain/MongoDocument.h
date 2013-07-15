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
        static QList<MongoDocumentPtr> fromBsonObj(const QList<mongo::BSONObj> &bsonObj);
		
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
        static QString buildJsonString(const QList<MongoDocumentPtr> &documents);

        static QString buildJsonString(const MongoDocumentPtr &documents);
    };

    class Concatenator
    {
    public:
        Concatenator();
        void append(const QString &data);
        QString build();

    private:
        QStringList _list;
        int _count;
    };
}
