#include "robomongo/core/settings/ReplicaSetSettings.h"

#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{

    ReplicaSetSettings::ReplicaSetSettings() 
        : _members(), _readPreference(ReadPreference::PRIMARY) 
    {}

    ReplicaSetSettings *ReplicaSetSettings::clone() const {
        ReplicaSetSettings *cloned = new ReplicaSetSettings(*this);
        return cloned;
    }

    QVariant ReplicaSetSettings::toVariant() const {
        QVariantMap map;
        int idx = 0;
        for (std::string const& str : _members) {
            map.insert(QString::number(idx), QtUtils::toQString(str));
            ++idx;
        }
        map.insert("readPreference", static_cast<int>(readPreference()));
        return map;
    }

    void ReplicaSetSettings::fromVariant(const QVariantMap &map) {
        // todo
        //setMembers()
        //setUserName(QtUtils::toStdString(map.value("userName").toString()));
        setReadPreference(static_cast<ReadPreference>(map.value("readPreference").toInt()));
    }
}
