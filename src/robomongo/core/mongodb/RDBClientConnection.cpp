#include "robomongo/core/mongodb/RDBClientConnection.h"

namespace{
    mongo::mutex _mutex( "RDBClientConnection::_mutex" );
}

namespace Robomongo
{
    RDBClientConnection::RDBClientConnection(bool _autoReconnect, mongo::DBClientReplicaSet* cp, double so_timeout) 
        :BaseClass(_autoReconnect,cp,so_timeout) {

    }

    bool RDBClientConnection::connect(const mongo::HostAndPort& server, bool sslSupport, const std::string &sslPEMKeyFile)
    {
        mongo::scoped_lock lk(_mutex);
        std::string err;
        mongo::cmdLine.sslOnNormalPorts = sslSupport;
        mongo::cmdLine.sslPEMKeyFile = sslPEMKeyFile;
        return BaseClass::connect(server,err);
    }
}
