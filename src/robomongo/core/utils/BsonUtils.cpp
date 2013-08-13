#include "robomongo/core/utils/BsonUtils.h"

#include <mongo/client/dbclient.h>
#include "robomongo/core/HexUtils.h"
#include "mongo/util/base64.h"
#include "robomongo/shell/db/ptimeutil.h"

using namespace mongo;
namespace Robomongo
{
    namespace BsonUtils
    {
        namespace detail
        {
            template<>
            mongo::BSONObj getField<mongo::BSONObj>(const mongo::BSONElement &elem) 
            {
                mongo::BSONObj res;
                try
                {
                   res = elem.Obj();
                }
                catch(const UserException &)
                {

                }
                return res;
            }

            template<>
            bool getField<bool>(const mongo::BSONElement &elem)
            {
                return elem.Bool();
            }

            template<>
            std::string getField<std::string>(const mongo::BSONElement &elem)
            {
                return elem.String();
            }

            template<>
            int getField<int>(const mongo::BSONElement &elem)
            {
                return elem.Int();
            }
        }

        std::string jsonString(BSONObj &obj, JsonStringFormat format, int pretty, UUIDEncoding uuidEncoding)
        {
            if ( obj.isEmpty() ) return "{}";

            StringBuilder s;
            s << "{";
            BSONObjIterator i(obj);
            BSONElement e = i.next();
            if ( !e.eoo() ){
                while ( 1 ) {
                    if ( pretty ) {
                        s << '\n';
                        for( int x = 0; x < pretty; x++ ){
                            s << "    ";
                        }
                    }
                    else {
                        s << " ";
                    }
                    s << jsonString(e, format, true, pretty?pretty+1:0, uuidEncoding);
                    e = i.next();

                    if (e.eoo()) {
                        s << '\n';
                        for( int x = 0; x < pretty - 1; x++ ){
                            s << "    ";
                        }
                        s << "}";
                        break;
                    }

                    s << ",";
                }
            }
            return s.str();
        }

        std::string jsonString(BSONElement &elem, JsonStringFormat format, bool includeFieldNames, int pretty, UUIDEncoding uuidEncoding)
        {
            BSONType t = elem.type();
            if ( t == Undefined )
                return "undefined";

            stringstream s;
            if ( includeFieldNames )
                s << '"' << escape( elem.fieldName() ) << "\" : ";

            switch ( t ) {
            case mongo::String:
            case Symbol:
                s << '"' << escape( string(elem.valuestr(), elem.valuestrsize()-1) ) << '"';
                break;
            case NumberLong:
                s << "NumberLong(" << elem._numberLong() << ")";
                break;
            case NumberInt:
            case NumberDouble:
                {
                    int sign=0;
                    if ( elem.number() >= -numeric_limits< double >::max() &&
                            elem.number() <= numeric_limits< double >::max() ) {
                        s.precision( 16 );
                        s << elem.number();
                    }
                    else if ( mongo::isNaN(elem.number()) ) {
                        s << "NaN";
                    }
                    else if ( mongo::isInf(elem.number(), &sign) ) {
                        s << ( sign == 1 ? "Infinity" : "-Infinity");
                    }
                    else {
                        StringBuilder ss;
                        ss << "Number " << elem.number() << " cannot be represented in JSON";
                        string message = ss.str();
                        //massert( 10311 ,  message.c_str(), false );
                    }
                    break;
                }
            case mongo::Bool:
                s << ( elem.boolean() ? "true" : "false" );
                break;
            case jstNULL:
                s << "null";
                break;
            case Object: {
                BSONObj obj = elem.embeddedObject();
                s << jsonString(obj, format, pretty, uuidEncoding);
                }
                break;
            case mongo::Array: {
                if ( elem.embeddedObject().isEmpty() ) {
                    s << "[]";
                    break;
                }
                s << "[ ";
                BSONObjIterator i( elem.embeddedObject() );
                BSONElement e = i.next();
                if ( !e.eoo() ) {
                    int count = 0;
                    while ( 1 ) {
                        if( pretty ) {
                            s << '\n';
                            for( int x = 0; x < pretty; x++ )
                                s << "    ";
                        }

                        if (strtol(e.fieldName(), 0, 10) > count) {
                            s << "undefined";
                        }
                        else {
                            s << jsonString(e, format, false, pretty?pretty+1:0, uuidEncoding);
                            e = i.next();
                        }
                        count++;
                        if ( e.eoo() ) {
                            s << '\n';
                            for( int x = 0; x < pretty - 1; x++ )
                                s << "    ";
                            s << "]";
                            break;
                        }
                        s << ", ";
                    }
                }
                //s << " ]";
                break;
            }
            case DBRef: {
                mongo::OID *x = (mongo::OID *) (elem.valuestr() + elem.valuestrsize());
                if ( format == TenGen )
                    s << "Dbref( ";
                else
                    s << "{ \"$ref\" : ";
                s << '"' << elem.valuestr() << "\", ";
                if ( format != TenGen )
                    s << "\"$id\" : ";
                s << '"' << *x << "\" ";
                if ( format == TenGen )
                    s << ')';
                else
                    s << '}';
                break;
            }
            case jstOID:
                if ( format == TenGen ) {
                    s << "ObjectId(";
                }
                else {
                    s << "{ \"$oid\" : ";
                }
                s << '"' << elem.__oid() << '"';
                if ( format == TenGen ) {
                    s << ")";
                }
                else {
                    s << " }";
                }
                break;
            case BinData: {
                int len = *(int *)( elem.value() );
                BinDataType type = BinDataType( *(char *)( (int *)( elem.value() ) + 1 ) );

                if (type == mongo::bdtUUID || type == mongo::newUUID) {
                    s << HexUtils::formatUuid(elem, uuidEncoding);
                    break;
                }

                s << "{ \"$binary\" : \"";
                char *start = ( char * )( elem.value() ) + sizeof( int ) + 1;
                base64::encode( s , start , len );
                s << "\", \"$type\" : \"" << hex;
                s.width( 2 );
                s.fill( '0' );
                s << type << dec;
                s << "\" }";
                break;
            }
            case mongo::Date:
                if ( format == Strict )
                    s << "{ \"$date\" : ";
                else
                    s << "ISODate(";

                if( pretty ) {
                    Date_t d = elem.date();

                    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
                    boost::posix_time::time_duration diff = boost::posix_time::millisec(d.millis);
                    boost::posix_time::ptime time = epoch + diff;
                    std::string timestr = miutil::isotimeString(time, true, true);
                    s << '"' << timestr << '"';
                }
                else
                    s << elem.date();

                if ( format == Strict )
                    s << " }";
                else
                    s << ")";
                break;
            case RegEx:
                if ( format == Strict ) {
                    s << "{ \"$regex\" : \"" << escape( elem.regex() );
                    s << "\", \"$options\" : \"" << elem.regexFlags() << "\" }";
                }
                else {
                    s << "/" << escape( elem.regex() , true ) << "/";
                    // FIXME Worry about alpha order?
                    for ( const char *f = elem.regexFlags(); *f; ++f ) {
                        switch ( *f ) {
                        case 'g':
                        case 'i':
                        case 'm':
                            s << *f;
                        default:
                            break;
                        }
                    }
                }
                break;

            case CodeWScope: {
                BSONObj scope = elem.codeWScopeObject();
                if ( ! scope.isEmpty() ) {
                    s << "{ \"$code\" : " << elem._asCode() << " , "
                      << " \"$scope\" : " << scope.jsonString() << " }";
                    break;
                }
            }

            case Code:
                s << elem._asCode();
                break;

            case Timestamp:
                if ( format == TenGen ) {
                    s << "Timestamp(" << ( elem.timestampTime() / 1000 ) << ", " << elem.timestampInc() << ")";
                }
                else {
                    s << "{ \"$timestamp\" : { \"t\" : " << ( elem.timestampTime() / 1000 ) << ", \"i\" : " << elem.timestampInc() << " } }";
                }
                break;

            case MinKey:
                s << "{ \"$minKey\" : 1 }";
                break;

            case MaxKey:
                s << "{ \"$maxKey\" : 1 }";
                break;

            default:
                StringBuilder ss;
                ss << "Cannot create a properly formatted JSON string with "
                   << "element: " << elem.toString() << " of type: " << elem.type();
                string message = ss.str();
                //massert( 10312 ,  message.c_str(), false );
            }
            return s.str();
        }
    }
}
