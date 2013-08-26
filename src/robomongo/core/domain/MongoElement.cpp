#include "robomongo/core/domain/MongoElement.h"
#include <mongo/client/dbclient.h>
#include <QStringBuilder>

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/HexUtils.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/shell/db/ptimeutil.h"

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
            _stringValue = QtUtils::toQString(con.build());
        }
        return _stringValue;
    }

    /*
    ** Get field name
    */
    std::string MongoElement::fieldName() const
    {
        return _bsonElement.fieldName();
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
            con.append(QtUtils::toStdString<std::string>(QString::number(_bsonElement.Double(),'g',14)));
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

                con.append(std::string(_bsonElement.valuestr(), _bsonElement.valuestrsize() - 1));
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
                    con.append(uuid);
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
                std::string idValue = _bsonElement.OID().toString();
                char buff[256]={0};
                sprintf(buff,"ObjectId(\"%s\")",idValue.c_str());
                con.append(buff);
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

            std::string date = miutil::isotimeString(time,false,AppRegistry::instance().settingsManager()->timeZone()==LocalTime);

            con.append(date);
            break;
        }

        /** null type */
        case jstNULL:
            con.append("<null>");
            break;

        /** regular expression, a pattern with options */
        case RegEx:
            {
                con.append("/" + std::string(_bsonElement.regex()) + "/");

                for ( const char *f = _bsonElement.regexFlags(); *f; ++f ) {
                    switch ( *f ) {
                    case 'g':
                    case 'i':
                    case 'm':
                        {
                            std::string str;
                            str+=*f;
                            con.append(str);
                        }
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
            con.append(_bsonElement._asCode());
            break;

        /** a programming language (e.g., Python) symbol */
        case Symbol:
            con.append(std::string(_bsonElement.valuestr(), _bsonElement.valuestrsize() - 1));
            break;

        /** javascript code that can execute on the database server, with SavedContext */
        case CodeWScope:
            {
                mongo::BSONObj scope = _bsonElement.codeWScopeObject();
                if (!scope.isEmpty() ) {
                    con.append(_bsonElement._asCode());
                    break;
                }
            }
            break;

        /** 32 bit signed integer */
        case NumberInt:
            {
                char num[8]={0};
                sprintf(num,"%d",_bsonElement.Int());
                con.append(num);
                break;
            }           

        /** Updated to a Date with value next OpTime on insert */
        case Timestamp:
            {
                Date_t date = _bsonElement.timestampTime();
                unsigned long long millis = date.millis;
                if ((long long)millis >= 0 &&
                    ((long long)millis/1000) < (std::numeric_limits<time_t>::max)()) {
                        con.append(date.toString());
                }
                break;
            }

        /** 64 bit integer */
        case NumberLong:
            {
                char num[16]={0};
                sprintf(num,"%d",_bsonElement.Long());
                con.append(num);
                break; 
            }
        default:
            con.append("<unsupported>");
            break;
        }
    }
}

