#ifndef MONGOELEMENT_H
#define MONGOELEMENT_H

#include <QObject>
#include "Core.h"
#include "mongo/client/dbclient.h"

using namespace mongo;

namespace Robomongo
{
	class Concatenator;

	/*
	** Wrapper around BSONElement
	*/
	class MongoElement : public QObject 
	{
		Q_OBJECT

	private:

		/*
		** Native BSONElement this MongoElement represents
		*/
		BSONElement _bsonElement;

		/*
		** Field Name
		*/
		QString _fieldName;

		/*
		** String value if this element
		*/
		QString _stringValue;

	public:
		
		/*
		** Create instance of MongoElement from BSONElement
		*/
		MongoElement(BSONElement bsonElement);

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

		/*
		** Check if this element is a BSON Object
		*/
		bool isDocument() const { return _bsonElement.isABSONObj(); }

		/*
		** Check if this element is an BSON array
		*/
		bool isArray() const { return _bsonElement.isABSONObj() && _bsonElement.type() == Array; }

		/*
		** Check if this element is a BSON String
		*/
		bool isString() const { return _bsonElement.type() == String; }

		/*
		** Return bson type
		*/
		BSONType type() const { return _bsonElement.type(); }

		/*
		** Return MongoDocument of this element (you should check that this IS document before)
		*/
		MongoDocument * asDocument();

		/*
		** Build Json string that represent this element.
		*/
		void buildJsonString(Concatenator * con);

		BSONElement bsonElement() const { return _bsonElement; }
	};
}


#endif // MONGOELEMENT_H
