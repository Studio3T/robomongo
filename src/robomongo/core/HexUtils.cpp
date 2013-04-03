#include "robomongo/core/HexUtils.h"

#include <mongo/util/hex.h>
#include <pcre/pcrecpp.h>

QString Robomongo::HexUtils::toHexLower(const void *raw, int len)
{
    std::string stdstr = mongo::toHexLower(raw, len);
    return QString::fromStdString(stdstr);
}

QString Robomongo::HexUtils::toHexLower(const char *raw, int len)
{
    const void* in = reinterpret_cast<const void*>(raw);
    return toHexLower(in, len);
}

const char *Robomongo::HexUtils::fromHex(const std::string &s, int *outBytes)
{
    const int size = s.size();
    if (size % 2 != 0)
        return NULL;

    const int bytes = size / 2; // number of bytes
    char *data = new char[bytes];

    const char *p = s.c_str();
    for( size_t i = 0; i < bytes; i++ ) {
        data[i] = mongo::fromHex(p);
        p += 2;
    }

    *outBytes = bytes;
    return data;
}

const char *Robomongo::HexUtils::fromHex(const QString &str, int *outBytes)
{
    std::string stdstr = str.toStdString();
    return fromHex(stdstr, outBytes);
}

std::string Robomongo::HexUtils::hexToUuid(const std::string &hex)
{
    std::string uuid = hex.substr(0, 8) + '-' + hex.substr(8, 4) + '-' + hex.substr(12, 4) + '-' + hex.substr(16, 4) + '-' + hex.substr(20, 12);
    return uuid;
}

std::string Robomongo::HexUtils::hexToCSharpUuid(const std::string &hex)
{
    std::string a = hex.substr(6, 2) + hex.substr(4, 2) + hex.substr(2, 2) + hex.substr(0, 2);
    std::string b = hex.substr(10, 2) + hex.substr(8, 2);
    std::string c = hex.substr(14, 2) + hex.substr(12, 2);
    std::string d = hex.substr(16, 16);
    std::string temp = a + b + c + d;
    std::string uuid = temp.substr(0, 8) + '-' + temp.substr(8, 4) + '-' + temp.substr(12, 4) + '-' + temp.substr(16, 4) + '-' + temp.substr(20, 12);
    return uuid;
}

std::string Robomongo::HexUtils::hexToJavaUuid(const std::string &hex)
{
    std::string msb = hex.substr(0, 16);
    std::string lsb = hex.substr(16, 16);
    msb = msb.substr(14, 2) + msb.substr(12, 2) + msb.substr(10, 2) + msb.substr(8, 2) + msb.substr(6, 2) + msb.substr(4, 2) + msb.substr(2, 2) + msb.substr(0, 2);
    lsb = lsb.substr(14, 2) + lsb.substr(12, 2) + lsb.substr(10, 2) + lsb.substr(8, 2) + lsb.substr(6, 2) + lsb.substr(4, 2) + lsb.substr(2, 2) + lsb.substr(0, 2);
    std::string temp = msb + lsb;
    std::string uuid = temp.substr(0, 8) + '-' + temp.substr(8, 4) + '-' + temp.substr(12, 4) + '-' + temp.substr(16, 4) + '-' + temp.substr(20, 12);
    return uuid;
}

std::string Robomongo::HexUtils::hexToPythonUuid(const std::string &hex)
{
    return hexToUuid(hex);
}

std::string Robomongo::HexUtils::uuidToHex(const std::string &uuid)
{
    // remove extra characters
    std::string hex = uuid;
    pcrecpp::RE re("[{}-]");
    re.GlobalReplace("", &hex);

    return hex;
}

std::string Robomongo::HexUtils::csharpUuidToHex(const std::string &uuid)
{
    // remove extra characters
    std::string hex = uuid;
    pcrecpp::RE re("[{}-]");
    re.GlobalReplace("", &hex);

    std::string a = hex.substr(6, 2) + hex.substr(4, 2) + hex.substr(2, 2) + hex.substr(0, 2);
    std::string b = hex.substr(10, 2) + hex.substr(8, 2);
    std::string c = hex.substr(14, 2) + hex.substr(12, 2);
    std::string d = hex.substr(16, 16);
    std::string result = a + b + c + d;
    return result;
}

std::string Robomongo::HexUtils::javaUuidToHex(const std::string &uuid)
{
    // remove extra characters
    std::string hex = uuid;
    pcrecpp::RE re("[{}-]");
    re.GlobalReplace("", &hex);

    std::string msb = hex.substr(0, 16);
    std::string lsb = hex.substr(16, 16);
    msb = msb.substr(14, 2) + msb.substr(12, 2) + msb.substr(10, 2) + msb.substr(8, 2) + msb.substr(6, 2) + msb.substr(4, 2) + msb.substr(2, 2) + msb.substr(0, 2);
    lsb = lsb.substr(14, 2) + lsb.substr(12, 2) + lsb.substr(10, 2) + lsb.substr(8, 2) + lsb.substr(6, 2) + lsb.substr(4, 2) + lsb.substr(2, 2) + lsb.substr(0, 2);
    std::string result = msb + lsb;
    return result;
}

std::string Robomongo::HexUtils::pythonUuidToHex(const std::string &uuid)
{
    return uuidToHex(uuid);
}
