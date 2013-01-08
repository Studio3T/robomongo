#include "robomongo/core/settings/CredentialSettings.h"

using namespace Robomongo;

CredentialSettings::CredentialSettings() :
    _enabled(false)
{
}

CredentialSettings::CredentialSettings(const QVariantMap &map) :
    _enabled(false)
{
    fromVariant(map);
}

/**
 * @brief Clones credential settings.
 */
CredentialSettings *CredentialSettings::clone() const
{
    CredentialSettings *cloned = new CredentialSettings();
    cloned->setUserName(userName());
    cloned->setUserPassword(userPassword());
    cloned->setDatabaseName(databaseName());
    cloned->setEnabled(enabled());
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

void CredentialSettings::fromVariant(QVariantMap map)
{
    setUserName(map.value("userName").toString());
    setUserPassword(map.value("userPassword").toString());
    setDatabaseName(map.value("databaseName").toString());
    setEnabled(map.value("enabled").toBool());
}
