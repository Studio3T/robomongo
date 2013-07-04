#pragma once
#include <QString>
#include "robomongo/core/domain/Enums.h"
#include <mongo/bson/bsonelement.h>

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
    namespace HexUtils
    {
        bool isHexString(const std::string &hex);
        QString toHexLower(const void* inRaw, int len);
        QString toHexLower(const char* inRaw, int len);
        std::string toStdHexLower(const char *raw, int len);
        /**
         * @param str: data in hex format.
         * @param outBytes: out param - number of bytes in array.
         * @return array of bytes, with "outBytes" length.
         */
        const char *fromHex(const std::string &str, int *outBytes);
        const char *fromHex(const QString &str, int *outBytes);
        std::string hexToUuid(const std::string &hex, UUIDEncoding encoding);
        std::string hexToUuid(const std::string &hex);
        std::string hexToCSharpUuid(const std::string &hex);
        std::string hexToJavaUuid(const std::string &hex);
        std::string hexToPythonUuid(const std::string &hex);
        /**
         * @return empty string, if invalid UUID.
         */
        std::string uuidToHex(const std::string &uuid, UUIDEncoding encoding);
        std::string uuidToHex(const std::string &uuid);
        std::string csharpUuidToHex(const std::string &uuid);
        std::string javaUuidToHex(const std::string &uuid);
        std::string pythonUuidToHex(const std::string &uuid);
        std::string formatUuid(mongo::BSONElement &element, UUIDEncoding encoding);
    }
}
