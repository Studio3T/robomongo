#pragma once

#include <QObject>
#include <mongo/client/dbclient.h>

#include "robomongo/core/Core.h"

namespace Robomongo
{
	class Concatenator;
    class SettingsManager;

	/*
	** Wrapper around BSONElement
	*/
	class MongoElement : public QObject 
	{
		Q_OBJECT

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
		QString fieldName();

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
        void buildJsonString(Concatenator &con);

        mongo::BSONElement bsonElement() const { return _bsonElement; }

    private:
        /*
        ** Native BSONElement this MongoElement represents
        */
        mongo::BSONElement _bsonElement;

        /*
        ** Field Name
        */
        QString _fieldName;

        /*
        ** String value if this element
        */
        QString _stringValue;

        SettingsManager *_settingsManager;
	};
}
