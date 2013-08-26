#pragma once
#include <QString>
#include <mongo/bson/bsonobj.h>
#include "robomongo/core/Core.h"

namespace Robomongo
{
    /*
    ** Wrapper around BSONElement
    */
    class MongoElement 
    {
    public:

        /*
        ** Create instance of MongoElement from BSONElement
        */
        MongoElement(mongo::BSONElement bsonElement);

        /*
        ** String value. Only for Simple types
        */
        QString stringValue();

        /*
        ** Get field name
        */
        std::string fieldName() const;

        /*
        ** Check if this element is of simple type
        */
        bool isSimpleType() const { return _bsonElement.isSimpleType(); }

        /**
        * @brief Check if this element is of UUID type
        */
        bool isUuidType() const {
        if (_bsonElement.type() != mongo::BinData)
        return false;

        mongo::BinDataType binType = _bsonElement.binDataType();
        return (binType == mongo::newUUID || binType == mongo::bdtUUID);
        }

        /*
        ** Check if this element is a BSON Object
        */
        bool isDocument() const { return _bsonElement.isABSONObj(); }

        /*
        ** Check if this element is an BSON array
        */
        bool isArray() const { return _bsonElement.isABSONObj() && _bsonElement.type() == mongo::Array; }

        /*
        ** Check if this element is a BSON String
        */
        bool isString() const { return _bsonElement.type() == mongo::String; }

        /*
        ** Return bson type
        */
        mongo::BSONType type() const { return _bsonElement.type(); }

        /*
        ** Return MongoDocument of this element (you should check that this IS document before)
        */
        MongoDocumentPtr asDocument();

        /*
        ** Build Json string that represent this element.
        */
        void buildJsonString(std::string &con);

        mongo::BSONElement bsonElement() const { return _bsonElement; }

    private:

        /*
        ** Native BSONElement this MongoElement represents
        */
        mongo::BSONElement _bsonElement;

        /*
        ** String value if this element
        */
        QString _stringValue;
    };
}
