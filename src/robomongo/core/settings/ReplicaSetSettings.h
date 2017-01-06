#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <mongo/util/net/hostandport.h>
#include <mongo/client/mongo_uri.h>

namespace Robomongo
{
    class ReplicaSetSettings
    {
    public:
        enum class ReadPreference
        {
            PRIMARY             = 0,
            PRIMARY_PREFERRED   = 1
        };

        ReplicaSetSettings();
        
        ReplicaSetSettings(const mongo::MongoURI& uri);

        /**
         * Clones Replica Set settings.
         */
        ReplicaSetSettings* clone() const;

        /**
         * Converts to QVariantMap
         */
        QVariant toVariant() const;
        void fromVariant(const QVariantMap &map);

        // Getters
        std::string setName() const { return _setName; }
        const std::vector<const std::string>& members() const { return _members; }
        const std::vector<mongo::HostAndPort> membersToHostAndPort() const;
        ReadPreference readPreference() const { return _readPreference; }

        // Setters
        void setSetName(const std::string& setName) { _setName = setName; }
        void setMembers(const std::vector<const std::string>& members);
        void setMembers(const std::vector<std::pair<std::string,bool>>& membersAndHealts);
        void deleteAllMembers() { _members.clear(); }
        void setReadPreference(ReadPreference readPreference) { _readPreference = readPreference; }


    private:
        std::string _setName;
        std::vector<const std::string> _members;
        ReadPreference _readPreference;
    };
}
