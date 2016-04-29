#include "robomongo/core/settings/SslSettings.h"

#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    SslSettings::SslSettings() :
            _enabled(false) { }

    SslSettings *SslSettings::clone() const {
        SslSettings *cloned = new SslSettings(*this);
        return cloned;
    }

    QVariant SslSettings::toVariant() const {
        QVariantMap map;
        map.insert("pemKeyFile", QtUtils::toQString(pemKeyFile()));
        map.insert("enabled", enabled());
        return map;
    }

    void SslSettings::fromVariant(const QVariantMap &map) {
        setPemKeyFile(QtUtils::toStdString(map.value("pemKeyFile").toString()));
        setEnabled(map.value("enabled").toBool());
    }
}
