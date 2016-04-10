#pragma once

#include <mongo/bson/bsonelement.h>
#include <mongo/bson/bsonobj.h>

#include "robomongo/core/Enums.h"

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
            struct bson_convert_traits<mongo::Array>
            {
                typedef std::vector<mongo::BSONElement> type;
                enum { mongo_type = mongo::Array };
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
            type_t result = type_t();
            bool isEoo = !elem.eoo();
            if (isEoo) {
                result = detail::getField<type_t>(elem);
            }
            return result;
        }

        template<mongo::BSONType BSONType_t>
        typename detail::bson_convert_traits<BSONType_t>::type getField(const mongo::BSONObj &obj, const char *data)
        {
            mongo::BSONElement elem = obj.getField(data);
            return bsonelement_cast<typename detail::bson_convert_traits<BSONType_t>::type>(elem);
        }

        std::string jsonString(const mongo::BSONObj &obj, mongo::JsonStringFormat format, int pretty,
            UUIDEncoding uuidEncoding, SupportedTimes timeFormat, bool isArray = false);

        std::string jsonString(const mongo::BSONElement &elem, mongo::JsonStringFormat format, bool includeFieldNames, int pretty,
            UUIDEncoding uuidEncoding, SupportedTimes timeFormat, bool isArray = false);

        bool isArray(const mongo::BSONElement &elem);
        bool isArray(mongo::BSONType type);
        bool isDocument(const mongo::BSONElement &elem);
        bool isDocument(mongo::BSONType type);

        bool isSimpleType(const mongo::BSONType type);
        bool isUuidType(const mongo::BSONType type, mongo::BinDataType binDataType);
        bool isSimpleType(const mongo::BSONElement &elem);
        bool isUuidType(const mongo::BSONElement &elem);

        const char* BSONTypeToString(mongo::BSONType type, mongo::BinDataType binDataType, UUIDEncoding uuidEncoding);

        void buildJsonString(const mongo::BSONObj &obj, std::string &con, UUIDEncoding uuid, SupportedTimes tz);
        void buildJsonString(const mongo::BSONElement &elem, std::string &con, UUIDEncoding uuid, SupportedTimes tz);

        mongo::BSONElement indexOf(const mongo::BSONObj &doc, int index);
        int elementsCount(const mongo::BSONObj &doc);
    }
}

