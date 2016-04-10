#include "robomongo/core/HexUtils.h"

#include <mongo/util/hex.h>
#include <pcrecpp.h>
#include <iostream>

namespace Robomongo
{
    namespace HexUtils
    {
        bool isHexString(const std::string &str)
        {
            std::size_t i;
            for (i = 0; i < str.size(); i++) {
                if (!isxdigit(str[i])) {
                    return false;
                }
            }
            return true;
        }

        std::string toStdHexLower(const char *raw, int len)
        {
            const void* in = reinterpret_cast<const void*>(raw);
            std::string stdstr = mongo::toHexLower(in, len);
            return stdstr;
        }

        const char *fromHex(const std::string &s, int *outBytes)
        {
            const int size = s.size();
            if (size % 2 != 0)
                return NULL;

            const int bytes = size / 2; // number of bytes
            char *data = new char[bytes];

            const char *p = s.c_str();
            for (size_t i = 0; i < bytes; i++) {
                data[i] = mongo::fromHex(p);
                p += 2;
            }

            *outBytes = bytes;
            return data;
        }

        std::string hexToUuid(const std::string &hex, UUIDEncoding encoding)
        {
            switch(encoding) {
            case DefaultEncoding: return hexToUuid(hex);
            case JavaLegacy:      return hexToJavaUuid(hex);
            case CSharpLegacy:    return hexToCSharpUuid(hex);
            case PythonLegacy:    return hexToPythonUuid(hex);
            default:              return hexToUuid(hex);
            }
        }

        std::string hexToUuid(const std::string &hex)
        {
            std::string uuid = hex.substr(0, 8) + '-' + hex.substr(8, 4) + '-' + hex.substr(12, 4) + '-' + hex.substr(16, 4) + '-' + hex.substr(20, 12);
            return uuid;
        }

        std::string hexToCSharpUuid(const std::string &hex)
        {
            std::string a = hex.substr(6, 2) + hex.substr(4, 2) + hex.substr(2, 2) + hex.substr(0, 2);
            std::string b = hex.substr(10, 2) + hex.substr(8, 2);
            std::string c = hex.substr(14, 2) + hex.substr(12, 2);
            std::string d = hex.substr(16, 16);
            std::string temp = a + b + c + d;
            std::string uuid = temp.substr(0, 8) + '-' + temp.substr(8, 4) + '-' + temp.substr(12, 4) + '-' + temp.substr(16, 4) + '-' + temp.substr(20, 12);
            return uuid;
        }

        std::string hexToJavaUuid(const std::string &hex)
        {
            std::string msb = hex.substr(0, 16);
            std::string lsb = hex.substr(16, 16);
            msb = msb.substr(14, 2) + msb.substr(12, 2) + msb.substr(10, 2) + msb.substr(8, 2) + msb.substr(6, 2) + msb.substr(4, 2) + msb.substr(2, 2) + msb.substr(0, 2);
            lsb = lsb.substr(14, 2) + lsb.substr(12, 2) + lsb.substr(10, 2) + lsb.substr(8, 2) + lsb.substr(6, 2) + lsb.substr(4, 2) + lsb.substr(2, 2) + lsb.substr(0, 2);
            std::string temp = msb + lsb;
            std::string uuid = temp.substr(0, 8) + '-' + temp.substr(8, 4) + '-' + temp.substr(12, 4) + '-' + temp.substr(16, 4) + '-' + temp.substr(20, 12);
            return uuid;
        }

        std::string hexToPythonUuid(const std::string &hex)
        {
            return hexToUuid(hex);
        }

        std::string uuidToHex(const std::string &uuid, Robomongo::UUIDEncoding encoding)
        {
            switch(encoding) {
            case DefaultEncoding: return uuidToHex(uuid);
            case JavaLegacy:      return javaUuidToHex(uuid);
            case CSharpLegacy:    return csharpUuidToHex(uuid);
            case PythonLegacy:    return pythonUuidToHex(uuid);
            default:              return uuidToHex(uuid);
            }
        }

        std::string uuidToHex(const std::string &uuid)
        {
            // remove extra characters
            std::string hex = uuid;
            pcrecpp::RE re("[{}-]");
            re.GlobalReplace("", &hex);

            if (hex.size() != 32)
                return "";

            return hex;
        }

        std::string csharpUuidToHex(const std::string &uuid)
        {
            // remove extra characters
            std::string hex = uuid;
            pcrecpp::RE re("[{}-]");
            re.GlobalReplace("", &hex);

            if (hex.size() != 32)
                return "";

            std::string a = hex.substr(6, 2) + hex.substr(4, 2) + hex.substr(2, 2) + hex.substr(0, 2);
            std::string b = hex.substr(10, 2) + hex.substr(8, 2);
            std::string c = hex.substr(14, 2) + hex.substr(12, 2);
            std::string d = hex.substr(16, 16);
            std::string result = a + b + c + d;
            return result;
        }

        std::string javaUuidToHex(const std::string &uuid)
        {
            // remove extra characters
            std::string hex = uuid;
            pcrecpp::RE re("[{}-]");
            re.GlobalReplace("", &hex);

            if (hex.size() != 32)
                return "";

            std::string msb = hex.substr(0, 16);
            std::string lsb = hex.substr(16, 16);
            msb = msb.substr(14, 2) + msb.substr(12, 2) + msb.substr(10, 2) + msb.substr(8, 2) + msb.substr(6, 2) + msb.substr(4, 2) + msb.substr(2, 2) + msb.substr(0, 2);
            lsb = lsb.substr(14, 2) + lsb.substr(12, 2) + lsb.substr(10, 2) + lsb.substr(8, 2) + lsb.substr(6, 2) + lsb.substr(4, 2) + lsb.substr(2, 2) + lsb.substr(0, 2);
            std::string result = msb + lsb;
            return result;
        }

        std::string pythonUuidToHex(const std::string &uuid)
        {
            return uuidToHex(uuid);
        }

        std::string formatUuid(const mongo::BSONElement &element, Robomongo::UUIDEncoding encoding)
        {
            mongo::BinDataType binType = element.binDataType();

            if (binType != mongo::newUUID && binType != mongo::bdtUUID)
                throw std::invalid_argument("Binary subtype should be 3 (bdtUUID) or 4 (newUUID)");

            int len;
            const char *data = element.binData(len);
            std::string hex = HexUtils::toStdHexLower(data, len);

            if (binType == mongo::bdtUUID) {
                std::string uuid = HexUtils::hexToUuid(hex, encoding);

                switch(encoding) {
                case DefaultEncoding: return "LUUID(\"" + uuid + "\")";
                case JavaLegacy:      return "JUUID(\"" + uuid + "\")";
                case CSharpLegacy:    return "NUUID(\"" + uuid + "\")";
                case PythonLegacy:    return "PYUUID(\"" + uuid + "\")";
                default:              return "LUUID(\"" + uuid + "\")";
                }
            } else {
                std::string uuid = HexUtils::hexToUuid(hex, DefaultEncoding);
                return "UUID(\"" + uuid + "\")";
            }
        }
    }
}
