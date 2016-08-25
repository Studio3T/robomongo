#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

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

        /**
         * Clones Replica Set settings.
         */
        ReplicaSetSettings* clone() const;

        /**
         * Converts to QVariantMap
         */
        QVariant toVariant() const;
        void fromVariant(const QVariantMap &map);

        //// todo: remove
        //void addMember(const std::string& str) { _members.push_back(str); }

        // Getters
        //
        const std::vector<std::string>& members() const { return _members; }
        ReadPreference readPreference() const { return _readPreference; }

        // Setters
        //
        void setMembers(const std::vector<std::string>& members) { _members = members; }
        void setReadPreference(ReadPreference readPreference) { _readPreference = readPreference; }

    private:
        std::vector<std::string> _members;
        ReadPreference _readPreference;
    };
}
