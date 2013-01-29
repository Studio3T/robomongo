#include "robomongo/core/engine/JsonBuilder.h"

#include "mongo/util/base64.h"

using namespace Robomongo;
using namespace mongo;

JsonBuilder::JsonBuilder()
{

}

std::string JsonBuilder::jsonString(BSONObj &obj, JsonStringFormat format, int pretty) const
{
    if ( obj.isEmpty() ) return "{}";

    StringBuilder s;
    s << "{ ";
    BSONObjIterator i(obj);
    BSONElement e = i.next();
    if ( !e.eoo() )
        while ( 1 ) {
            s << jsonString(e, format, true, pretty?pretty+1:0 );
            e = i.next();
            if ( e.eoo() )
                break;
            s << ",";
            if ( pretty ) {
                s << '\n';
                for( int x = 0; x < pretty; x++ )
                    s << "  ";
            }
            else {
                s << " ";
            }
        }
    s << " }";
    return s.str();
}

string JsonBuilder::jsonString(BSONElement &elem, JsonStringFormat format, bool includeFieldNames, int pretty) const
{
    BSONType t = elem.type();
    int sign;
    if ( t == Undefined )
        return "undefined";

    stringstream s;
    if ( includeFieldNames )
        s << '"' << escape( elem.fieldName() ) << "\" : ";
    switch ( elem.type() ) {
    case mongo::String:
    case Symbol:
        s << '"' << escape( string(elem.valuestr(), elem.valuestrsize()-1) ) << '"';
        break;
    case NumberLong:
        s << elem._numberLong();
        break;
    case NumberInt:
    case NumberDouble:
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
    case mongo::Bool:
        s << ( elem.boolean() ? "true" : "false" );
        break;
    case jstNULL:
        s << "null";
        break;
    case Object: {
        BSONObj obj = elem.embeddedObject();
        s << jsonString(obj, format, pretty );
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
                        s << "  ";
                }

                if (strtol(e.fieldName(), 0, 10) > count) {
                    s << "undefined";
                }
                else {
                    s << jsonString(e, format, false, pretty?pretty+1:0 );
                    e = i.next();
                }
                count++;
                if ( e.eoo() )
                    break;
                s << ", ";
            }
        }
        s << " ]";
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
            s << "ObjectId( ";
        }
        else {
            s << "{ \"$oid\" : ";
        }
        s << '"' << elem.__oid() << '"';
        if ( format == TenGen ) {
            s << " )";
        }
        else {
            s << " }";
        }
        break;
    case BinData: {
        int len = *(int *)( elem.value() );
        BinDataType type = BinDataType( *(char *)( (int *)( elem.value() ) + 1 ) );
        s << "{ \"$binary\" : \"";
        char *start = ( char * )( elem.value() ) + sizeof( int ) + 1;
        base64::encode( s , start , len );  //TODO: SIC!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
            s << "Date( ";
        if( pretty ) {
            Date_t d = elem.date();
            if( d == 0 ) s << '0';
            else
                s << '"' << elem.date().toString() << '"';
        }
        else
            s << elem.date();
        if ( format == Strict )
            s << " }";
        else
            s << " )";
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
        s << "{ \"t\" : " << elem.timestampTime() << " , \"i\" : " << elem.timestampInc() << " }";
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
