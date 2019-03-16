#include "robomongo/core/settings/CredentialSettings.h"

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/utils/RoboCrypt.h"

namespace Robomongo
{
    CredentialSettings::CredentialSettings() :
        _userName(),
        _userPassword(),
        _databaseName(),
        _mechanism(),
        _enabled(false)
    {

    }
    CredentialSettings::CredentialSettings(const QVariantMap &map) :
        _userName(QtUtils::toStdString(map.value("userName").toString())),
        _databaseName(QtUtils::toStdString(map.value("databaseName").toString())),
        _mechanism(QtUtils::toStdString(map.value("mechanism").toString())),
        _enabled(map.value("enabled").toBool())
    {
        // From version Robo 1.3 "userPasswordEncrypted" is used instead of "userPassword" in config. file
        if(map.contains("userPassword")) // Robo 1.2 and below
            _userPassword = map.value("userPassword").toString().toStdString();
        else if(map.contains("userPasswordEncrypted")) // From Robo 1.3
            _userPassword = RoboCrypt::decrypt(map.value("userPasswordEncrypted").toString().toStdString());
    }

    /**
     * @brief Clones credential settings.
     */
    CredentialSettings *CredentialSettings::clone() const
    {
        CredentialSettings *cloned = new CredentialSettings(*this);
        return cloned;
    }

    QVariant CredentialSettings::toVariant() const
    {
        QVariantMap map;
        map.insert("userName", QtUtils::toQString(userName()));
        map.insert("userPasswordEncrypted", userPassword().empty() ? "" :
                                            QtUtils::toQString(RoboCrypt::encrypt(userPassword())));
        map.insert("databaseName", QtUtils::toQString(databaseName()));
        map.insert("mechanism", QtUtils::toQString(mechanism()));
        map.insert("enabled", enabled());
        return map;
    }
}