#include "CredentialSettings.h"

using namespace Robomongo;

CredentialSettings::CredentialSettings(const QString &userName, const QString &userPassword, const QString &databaseName) :
    _userName(userName),
    _userPassword(userPassword),
    _databaseName(databaseName),
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
    return new CredentialSettings(userName(), userPassword(), databaseName());
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
