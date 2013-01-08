#include <QStringBuilder>

#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoDocument.h"


using namespace mongo;

namespace Robomongo
{
    /*
	** Create instance of MongoElement from BSONElement
	*/
    MongoElement::MongoElement(BSONElement bsonElement) : QObject()
	{
		_bsonElement = bsonElement;
	}

	/*
	** String value. Only for Simple types
	*/
	QString MongoElement::stringValue()
	{
		if(_stringValue.isNull())
		{
            Concatenator con;
			buildJsonString(con);
            _stringValue = con.build();
		}

		return _stringValue;
	}

	/*
	** Get field name
	*/
	QString MongoElement::fieldName()
	{
		if (_fieldName.isNull())
			_fieldName = QString::fromUtf8(_bsonElement.fieldName());

		return _fieldName;
	}

	/*
	** Return MongoDocument of this element (you should check that this IS document before)
	*/
    MongoDocumentPtr MongoElement::asDocument()
	{
        MongoDocument *doc = new MongoDocument(_bsonElement.Obj());
        return MongoDocumentPtr(doc);
	}

	/*
	** Build Json string that represent this element.
	*/
    void MongoElement::buildJsonString(Concatenator &con)
	{
//		QString & buff = *pBuff;

		switch (_bsonElement.type())
		{
		/** double precision floating point value */
		case NumberDouble:
            con.append(QString::number(_bsonElement.Double()));
			break;

		/** character string, stored in utf8 */
		case String:
			{
				/*
				** If you'll write:
				** 
				**   int valsize    = element.valuesize();
				**   int strsize    = element.valuestrsize();
				**   int bytescount = qstrlen(element.valuestr());
				**  
				** You'll get:
				**
				**   bytescount + 1 == strsize
				**   strsize + 4    == valsize
				**
				** So:
				**   bytescount + 5 == valsize
				**
				*/

				QString res = QString::fromUtf8(_bsonElement.valuestr(), _bsonElement.valuestrsize() - 1);
                con.append(res);
			}
			break;

		/** an embedded object */
		case Object:
			{
                MongoDocumentPtr doc = asDocument();
				doc->buildJsonString(con);
			}
			break;

		/** an embedded array */
		case Array:
			{
                MongoDocumentPtr doc = asDocument();
				doc->buildJsonString(con);
			}		
			break;

		/** binary data */
		case BinData:
            con.append("<binary>");
			break;

		/** Undefined type */
		case Undefined:
            con.append("<undefined>");
			break;

		/** ObjectId */
		case jstOID: 
            con.append(QString::fromStdString(_bsonElement.OID().toString()));
			break;

		/** boolean type */
		case Bool:
            con.append(_bsonElement.Bool() ? "true" : "false");
			break;

		/** date type */
		case Date:
        {
            unsigned long long millis = _bsonElement.Date().millis;
            if ((long long)millis >= 0 &&
               ((long long)millis/1000) < (std::numeric_limits<time_t>::max)()) {
                con.append(QString::fromStdString(_bsonElement.Date().toString()));
            }
			break;
        }

		/** null type */
		case jstNULL:
            con.append(QString("<null>"));
			break;

		/** regular expression, a pattern with options */
		case RegEx:
			break;

		/** deprecated / will be redesigned */
		case DBRef:
			break;

		/** deprecated / use CodeWScope */
		case Code:
			break;

		/** a programming language (e.g., Python) symbol */
		case Symbol:
			break;

		/** javascript code that can execute on the database server, with SavedContext */
		case CodeWScope:
			break;

		/** 32 bit signed integer */
		case NumberInt:
            con.append(QString::number(_bsonElement.Int()));
			break;

		/** Updated to a Date with value next OpTime on insert */
		case Timestamp:
        {
            Date_t date = _bsonElement.timestampTime();
            unsigned long long millis = date.millis;
            if ((long long)millis >= 0 &&
               ((long long)millis/1000) < (std::numeric_limits<time_t>::max)()) {
                con.append(QString::fromStdString(date.toString()));
            }
			break;
        }

		/** 64 bit integer */
		case NumberLong:
            con.append(QString::number(_bsonElement.Long()));
			break; 

		default:
            con.append("<unsupported>");
			break;
		}
	}
}

