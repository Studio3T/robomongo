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
            // todo: Add all supported read preferences
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
        std::string const& setNameUserEntered() { return _setNameUserEntered; }
        std::string const& cachedSetName() { return _cachedSetName; }
        std::vector<std::string> const& members() const { return _members; }
        std::vector<mongo::HostAndPort> membersToHostAndPort() const;

        std::set<mongo::HostAndPort> membersToHostAndPortAsSet() const
        {
            auto const& members = membersToHostAndPort();
            return std::set<mongo::HostAndPort>(members.begin(), members.end());
        }
        
        ReadPreference readPreference() const { return _readPreference; }

        // Setters
        void setSetNameUserEntered(const std::string& setName) { _setNameUserEntered = setName; }
        void setCachedSetName(const std::string& setName) { _cachedSetName = setName; }
        void setMembers(const std::vector<std::string>& members);
        void setMembers(const std::vector<std::pair<std::string,bool>>& membersAndHealts);
        void deleteAllMembers() { _members.clear(); }
        void setReadPreference(ReadPreference readPreference) { _readPreference = readPreference; }


    private:
        std::string _setNameUserEntered;
        std::string _cachedSetName;
        std::vector<std::string> _members;
        ReadPreference _readPreference = ReadPreference::PRIMARY;
    };
}
