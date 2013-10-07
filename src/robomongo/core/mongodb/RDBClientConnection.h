#pragma once
#include <mongo/client/dbclientinterface.h>

namespace Robomongo
{
    class RDBClientConnection: public mongo::DBClientConnection
    {
    public:
        RDBClientConnection(bool _autoReconnect=false, mongo::DBClientReplicaSet* cp=0, double so_timeout=0);
        typedef mongo::DBClientConnection BaseClass;
        bool connect(const mongo::HostAndPort& server, bool sslSupport, const std::string &sslPEMKeyFile);
    };
}
