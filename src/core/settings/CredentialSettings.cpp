#include "CredentialSettings.h"

using namespace Robomongo;

CredentialSettings::CredentialSettings(const QString &userName, const QString &userPassword, const QString &databaseName) :
    _userName(userName),
    _userPassword(userPassword),
    _databaseName(databaseName)
{

}

CredentialSettings::CredentialSettings(const QVariantMap &map)
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
    return map;
}

void CredentialSettings::fromVariant(QVariantMap map)
{
    setUserName(map.value("userName").toString());
    setUserPassword(map.value("userPassword").toString());
    setDatabaseName(map.value("databaseName").toString());
}
