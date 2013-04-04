#pragma once

#include <QString>
#include <mongo/client/dbclient.h>

#include "robomongo/core/Core.h"

namespace Robomongo
{

    /**
     * @brief HexUtils
     *
     *  Usage:
     *
     *  int bytes;
     *  const char *data = HexUtils::fromHex(QString("00112233445566778899aabbccddeeff"), &bytes);
     *  QString hex = HexUtils::toHexLower(data, bytes);
     *
     *  std::string csuuid = HexUtils::hexToCSharpUuid(hex.toStdString());
     *  std::string cshex  = HexUtils::csharpUuidToHex(csuuid);
     *
     *  std::string juuid = HexUtils::hexToJavaUuid(hex.toStdString());
     *  std::string jhex  = HexUtils::javaUuidToHex(juuid);
     *
     *  std::string puuid = HexUtils::hexToPythonUuid(hex.toStdString());
     *  std::string phex  = HexUtils::pythonUuidToHex(puuid);*
     *
     */
    class HexUtils
    {
    public:

        static bool isHexString(const std::string &hex);

        static QString toHexLower(const void* inRaw, int len);
        static QString toHexLower(const char* inRaw, int len);
        static std::string toStdHexLower(const char *raw, int len);

        /**
         * @param str: data in hex format.
         * @param outBytes: out param - number of bytes in array.
         * @return array of bytes, with "outBytes" length.
         */
        static const char *fromHex(const std::string &str, int *outBytes);
        static const char *fromHex(const QString &str, int *outBytes);

        static std::string hexToUuid(const std::string &hex, UUIDEncoding encoding);
        static std::string hexToUuid(const std::string &hex);
        static std::string hexToCSharpUuid(const std::string &hex);
        static std::string hexToJavaUuid(const std::string &hex);
        static std::string hexToPythonUuid(const std::string &hex);

        /**
         * @return empty string, if invalid UUID.
         */
        static std::string uuidToHex(const std::string &uuid, UUIDEncoding encoding);
        static std::string uuidToHex(const std::string &uuid);
        static std::string csharpUuidToHex(const std::string &uuid);
        static std::string javaUuidToHex(const std::string &uuid);
        static std::string pythonUuidToHex(const std::string &uuid);

        static std::string formatUuid(mongo::BSONElement &element, UUIDEncoding encoding);

    private: // use static members
        HexUtils() {}
    };
}
