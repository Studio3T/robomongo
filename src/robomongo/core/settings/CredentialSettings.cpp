#include "robomongo/core/settings/CredentialSettings.h"

#include "robomongo/core/utils/QtUtils.h"

#define USERNAME "userName"
#define USERPASSWORD "userPassword"
#define DATABASENAME "databaseName"
#define AUTOTENTIFICATION "enabled"

namespace Robomongo
{
   
    CredentialSettings::CredentialInfo::CredentialInfo():
        _userName(),
        _userPassword(),
        _databaseName()
    {
    }

    CredentialSettings::CredentialInfo::CredentialInfo(const std::string &userName, const std::string &userPassword, const std::string &databaseName):
        _userName(userName),
        _userPassword(userPassword),
        _databaseName(databaseName.empty() ? "admin" : databaseName )
    {
    }

    CredentialSettings::CredentialSettings() :
        _credentialInfo(),
        _enabled(false)
    {
    }

    CredentialSettings::CredentialSettings(const QVariantMap &map) :
        _credentialInfo(QtUtils::toStdString(map.value(USERNAME).toString()),
        QtUtils::toStdString(map.value(USERPASSWORD).toString()),
        QtUtils::toStdString(map.value(DATABASENAME).toString())),
        _enabled(map.value(AUTOTENTIFICATION).toBool())
    {
    }


    QVariantMap CredentialSettings::toVariant() const
    {
        QVariantMap map;
        map.insert(USERNAME, QtUtils::toQString(_credentialInfo._userName));
        map.insert(USERPASSWORD, QtUtils::toQString(_credentialInfo._userPassword));
        map.insert(DATABASENAME, QtUtils::toQString(_credentialInfo._databaseName));
        map.insert(AUTOTENTIFICATION, _enabled);
        return map;
    }
}
