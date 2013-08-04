#include "robomongo/core/settings/CredentialSettings.h"

namespace Robomongo
{
    CredentialSettings::CredentialSettings() :
        _userName(),
        _userPassword(),
        _databaseName(),
        _enabled(false)
    {

    }
    CredentialSettings::CredentialSettings(const QVariantMap &map) :
        _userName(map.value("userName").toString()),
        _userPassword(map.value("userPassword").toString()),
        _databaseName(map.value("databaseName").toString()),
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
        map.insert("userName", userName());
        map.insert("userPassword", userPassword());
        map.insert("databaseName", databaseName());
        map.insert("enabled", enabled());
        return map;
    }
}
