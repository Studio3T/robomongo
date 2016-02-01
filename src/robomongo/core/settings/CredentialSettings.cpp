#include "robomongo/core/settings/CredentialSettings.h"

#include "robomongo/core/utils/QtUtils.h"

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
        _userPassword(QtUtils::toStdString(map.value("userPassword").toString())),
        _databaseName(QtUtils::toStdString(map.value("databaseName").toString())),
        _mechanism(QtUtils::toStdString(map.value("mechanism").toString())),
        _enabled(map.value("enabled").toBool())
    {
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
        map.insert("userPassword", QtUtils::toQString(userPassword()));
        map.insert("databaseName", QtUtils::toQString(databaseName()));
        map.insert("mechanism", QtUtils::toQString(mechanism()));
        map.insert("enabled", enabled());
        return map;
    }
}
