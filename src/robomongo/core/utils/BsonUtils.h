#pragma once
#include <mongo/bson/bsonelement.h>
#include <mongo/bson/bsonobj.h>
#include "robomongo/core/domain/Enums.h"
namespace Robomongo
{
    namespace BsonUtils
    {
        namespace detail
        {
            template<mongo::BSONType BSONType_t>
            struct bson_convert_traits
            {
                typedef mongo::BSONObj type;
                enum { mongo_type = BSONType_t };
            };
            template<>
            struct bson_convert_traits<mongo::Bool>
            {
                typedef bool type;
                enum { mongo_type = mongo::Bool };
            };
            template<>
            struct bson_convert_traits<mongo::String>
            {
                typedef std::string type;
                enum { mongo_type = mongo::String };
            };
            template<>
            struct bson_convert_traits<mongo::NumberInt>
            {
                typedef int type;
                enum { mongo_type = mongo::NumberInt };
            };
            template<>
            struct bson_convert_traits<mongo::NumberDouble>
            {
                typedef double type;
                enum { mongo_type = mongo::NumberDouble };
            };
            template<>
            struct bson_convert_traits<mongo::NumberLong>
            {
                typedef long long type;
                enum { mongo_type = mongo::NumberLong };
            };
            template<typename type_t>
            type_t getField(const mongo::BSONElement &elem);                  
        }

        template<typename type_t>
        type_t bsonelement_cast(const mongo::BSONElement &elem)
        {
            type_t result=type_t();
            bool isEoo = !elem.eoo();
            if (isEoo){
                result = detail::getField<type_t>(elem);
            }
            return result;
        }

        template<mongo::BSONType BSONType_t>
        typename detail::bson_convert_traits<BSONType_t>::type getField(const mongo::BSONObj &obj,const char *data)
        {
            mongo::BSONElement elem = obj.getField(data);
            return bsonelement_cast<typename detail::bson_convert_traits<BSONType_t>::type>(elem);
        }

        std::string jsonString(mongo::BSONObj &obj, mongo::JsonStringFormat format, int pretty, UUIDEncoding uuidEncoding, SupportedTimes timeFormat);
        std::string jsonString(mongo::BSONElement &elem, mongo::JsonStringFormat format, bool includeFieldNames, int pretty, UUIDEncoding uuidEncoding, SupportedTimes timeFormat);

        bool isArray(const mongo::BSONElement &elem);
        bool isDocument(const mongo::BSONElement &elem);
        bool isSimpleType(const mongo::BSONElement &elem);
        bool isUuidType(const mongo::BSONElement &elem);

        void buildJsonString(const mongo::BSONObj &obj,std::string &con, UUIDEncoding uuid,SupportedTimes tz);
        void buildJsonString(const mongo::BSONElement &elem,std::string &con, UUIDEncoding uuid,SupportedTimes tz);
    }
}

