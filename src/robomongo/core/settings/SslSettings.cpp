#include "robomongo/core/settings/SslSettings.h"

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/utils/RoboCrypt.h"

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
        map.insert("pemPassPhraseEncrypted", pemPassPhrase().empty() ? "" :
                                             QtUtils::toQString(RoboCrypt::encrypt(pemPassPhrase())));
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
        
        // From version Robo 1.3 "pemPassPhraseEncrypted" is used instead of "pemPassPhrase" in config. file
        if (map.contains("pemPassPhrase")) // Robo 1.2 and below
            setPemPassPhrase((map.value("pemPassPhrase").toString().toStdString()));
        else if (map.contains("pemPassPhraseEncrypted")) // From Robo 1.3
            setPemPassPhrase(RoboCrypt::decrypt((map.value("pemPassPhraseEncrypted").toString().toStdString())));

        setUseAdvancedOptions(map.value("useAdvancedOptions").toBool());
        setCrlFile(QtUtils::toStdString(map.value("crlFile").toString()));
        setAllowInvalidHostnames(map.value("allowInvalidHostnames").toBool());
        setAllowInvalidCertificates(map.value("allowInvalidCertificates").toBool());
        setAskPassphrase(map.value("askPassphrase").toBool());
    }
}
