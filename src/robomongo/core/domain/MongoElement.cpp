#include "robomongo/core/domain/MongoElement.h"
#include <mongo/client/dbclient.h>
#include <QStringBuilder>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/HexUtils.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"

using namespace mongo;

namespace Robomongo
{
    /*
	** Create instance of MongoElement from BSONElement
	*/
    MongoElement::MongoElement(BSONElement bsonElement)
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
        return QString::fromUtf8(_bsonElement.fieldName());
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
            {
                mongo::BinDataType binType = _bsonElement.binDataType();
                if (binType == mongo::newUUID || binType == mongo::bdtUUID) {
                    std::string uuid = HexUtils::formatUuid(_bsonElement, AppRegistry::instance().settingsManager()->uuidEncoding());
                    con.append(QString::fromStdString(uuid));
                    break;
                }

                con.append("<binary>");
            }
            break;

        /** Undefined type */
        case Undefined:
            con.append("<undefined>");
            break;

        /** ObjectId */
        case jstOID:
            {
                QString idValue = QString::fromStdString(_bsonElement.OID().toString());
                QString objectId = QString("ObjectId(\"%1\")").arg(idValue);
                con.append(objectId);
            }
            break;

        /** boolean type */
        case Bool:
            con.append(_bsonElement.Bool() ? "true" : "false");
            break;

        /** date type */
        case Date:
        {
            long long ms = (long long) _bsonElement.Date().millis;

            boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
            boost::posix_time::time_duration diff = boost::posix_time::millisec(ms);
            boost::posix_time::ptime time = epoch + diff;



            std::stringstream strm;

            //boost::date_time::time_facet *timeFacet = new boost::date_time::time_facet("%a, %d %b %Y %H:%M:%S.%f GMT"); // "%Y---%m-%d %H:%M:%S"
            boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S");
            strm.imbue(std::locale(strm.getloc(), facet));
            strm << time;

            con.append(QString::fromStdString(strm.str()));
            break;

            /*
            // this code is left untill the upper one will stabilize
            unsigned long long millis = _bsonElement.Date().millis;
            if ((long long)millis >= 0 &&
            ((long long)millis/1000) < (std::numeric_limits<time_t>::max)()) {
            con.append(QString::fromStdString(_bsonElement.Date().toString()));
            }
            break;
            */
        }

        /** null type */
        case jstNULL:
            con.append(QString("<null>"));
            break;

        /** regular expression, a pattern with options */
        case RegEx:
            {
                con.append("/" + QString::fromUtf8(_bsonElement.regex()) + "/");

                for ( const char *f = _bsonElement.regexFlags(); *f; ++f ) {
                    switch ( *f ) {
                    case 'g':
                    case 'i':
                    case 'm':
                        con.append(QString(*f));
                    default:
                        break;
                    }
                }
            }
            break;

        /** deprecated / will be redesigned */
        case DBRef:
            break;

        /** deprecated / use CodeWScope */
        case Code:
            con.append(QString::fromUtf8(_bsonElement._asCode().data()));
            break;

        /** a programming language (e.g., Python) symbol */
        case Symbol:
            con.append(QString::fromUtf8(_bsonElement.valuestr(), _bsonElement.valuestrsize() - 1));
            break;

        /** javascript code that can execute on the database server, with SavedContext */
        case CodeWScope:
            {
                mongo::BSONObj scope = _bsonElement.codeWScopeObject();
                if (!scope.isEmpty() ) {
                    con.append(QString::fromUtf8(_bsonElement._asCode().data()));
                    break;
                }
            }
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

