#include "robomongo/core/settings/SshSettings.h"

#include "robomongo/core/utils/QtUtils.h"

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
        map.insert("userPassword", QtUtils::toQString(userPassword()));
        map.insert("privateKeyFile", QtUtils::toQString(privateKeyFile()));
        map.insert("publicKeyFile", QtUtils::toQString(publicKeyFile()));
        map.insert("passphrase", QtUtils::toQString(passphrase()));
        map.insert("method", QtUtils::toQString(authMethod()));
        map.insert("enabled", enabled());
        map.insert("askPassword", askPassword());
        return map;
    }

    void SshSettings::fromVariant(const QVariantMap &map) {
        setHost(QtUtils::toStdString(map.value("host").toString()));
        setPort(map.value("port").toInt());
        setUserName(QtUtils::toStdString(map.value("userName").toString()));
        setUserPassword(QtUtils::toStdString(map.value("userPassword").toString()));
        setPrivateKeyFile(QtUtils::toStdString(map.value("privateKeyFile").toString()));
        setPublicKeyFile(QtUtils::toStdString(map.value("publicKeyFile").toString()));
        setPassphrase(QtUtils::toStdString(map.value("passphrase").toString()));
        setAuthMethod(QtUtils::toStdString(map.value("method").toString()));
        setEnabled(map.value("enabled").toBool());
        setAskPassword(map.value("askPassword").toBool());
    }
}
