#include "robomongo/core/settings/SslSettings.h"

#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    SslSettings::SslSettings() :
        _sslEnabled(false), _caFile(""), _pemKeyFile(""),  _pemPassPhrase(""), _crlFile(""), 
        _allowInvalidHostnames(false),  _allowInvalidCertificates(false), _usePemFile(false),
        _useAdvancedOptions(false), _askPassphrase(false)
    {}

    SslSettings *SslSettings::clone() const 
    {
        SslSettings *cloned = new SslSettings(*this);
        return cloned;
    }

    QVariant SslSettings::toVariant() const 
    {
        QVariantMap map;
        map.insert("sslEnabled", sslEnabled());
        map.insert("caFile", QtUtils::toQString(caFile()));
        map.insert("usePemFile", usePemFile());
        map.insert("pemKeyFile", QtUtils::toQString(pemKeyFile()));
        map.insert("pemPassPhrase", QtUtils::toQString(pemPassPhrase()));
        map.insert("useAdvancedOptions", useAdvancedOptions());
        map.insert("crlFile", QtUtils::toQString(crlFile()));
        map.insert("allowInvalidHostnames", allowInvalidHostnames());
        map.insert("allowInvalidCertificates", allowInvalidCertificates());
        map.insert("askPassphrase", askPassphrase());

        return map;
    }

    void SslSettings::fromVariant(const QVariantMap &map) 
    {
        enableSSL(map.value("sslEnabled").toBool());
        setCaFile(QtUtils::toStdString(map.value("caFile").toString()));
        setUsePemFile(map.value("usePemFile").toBool());
        setPemKeyFile(QtUtils::toStdString(map.value("pemKeyFile").toString()));
        setPemPassPhrase(QtUtils::toStdString(map.value("pemPassPhrase").toString()));
        setUseAdvancedOptions(map.value("useAdvancedOptions").toBool());
        setCrlFile(QtUtils::toStdString(map.value("crlFile").toString()));
        setAllowInvalidHostnames(map.value("allowInvalidHostnames").toBool());
        setAllowInvalidCertificates(map.value("allowInvalidCertificates").toBool());
        setAskPassphrase(map.value("askPassphrase").toBool());
    }
}
