#include "robomongo/core/settings/SshSettings.h"

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/utils/RoboCrypt.h"

namespace Robomongo
{
    SshSettings::SshSettings() :
        _port(22),
        _authMethod("publickey"),
        _enabled(false),
        _askPassword(false),
        _logLevel(1) {

    }

    SshSettings *SshSettings::clone() const {
        SshSettings *cloned = new SshSettings(*this);
        return cloned;
    }

    QVariant SshSettings::toVariant() const {
        QVariantMap map;
        map.insert("host", QtUtils::toQString(host()));
        map.insert("port", port());
        map.insert("userName", QtUtils::toQString(userName()));
        map.insert("userPasswordEncrypted", userPassword().empty() ? "" : 
                                            QtUtils::toQString(RoboCrypt::encrypt(userPassword())));
        map.insert("privateKeyFile", QtUtils::toQString(privateKeyFile()));
        map.insert("publicKeyFile", QtUtils::toQString(publicKeyFile()));
        map.insert("passphraseEncrypted", passphrase().empty() ? "" : 
                                          QtUtils::toQString(RoboCrypt::encrypt(passphrase())));
        map.insert("method", QtUtils::toQString(authMethod()));
        map.insert("enabled", enabled());
        map.insert("askPassword", askPassword());
        return map;
    }

    void SshSettings::fromVariant(const QVariantMap &map) {
        setHost(QtUtils::toStdString(map.value("host").toString()));
        setPort(map.value("port").toInt());
        setUserName(QtUtils::toStdString(map.value("userName").toString()));

        // From version Robo 1.3 "userPasswordEncrypted" is used instead of "userPassword" in config. file
        if (map.contains("userPassword")) // Robo 1.2 and below
            setUserPassword((map.value("userPassword").toString().toStdString()));
        else if (map.contains("userPasswordEncrypted")) // From Robo 1.3
            setUserPassword(RoboCrypt::decrypt((map.value("userPasswordEncrypted").toString().toStdString())));

        setPrivateKeyFile(QtUtils::toStdString(map.value("privateKeyFile").toString()));
        setPublicKeyFile(QtUtils::toStdString(map.value("publicKeyFile").toString()));

        // From version Robo 1.3 "passphraseEncrypted" is used instead of "passphrase" in config. file
        if (map.contains("passphrase")) // Robo 1.2 and below
            setPassphrase((map.value("passphrase").toString().toStdString()));
        else if (map.contains("passphraseEncrypted")) // From Robo 1.3
            setPassphrase(RoboCrypt::decrypt((map.value("passphraseEncrypted").toString().toStdString())));

        setAuthMethod(QtUtils::toStdString(map.value("method").toString()));
        setEnabled(map.value("enabled").toBool());
        setAskPassword(map.value("askPassword").toBool());
    }
}
