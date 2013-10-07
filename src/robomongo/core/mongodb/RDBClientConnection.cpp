#include "robomongo/core/mongodb/RDBClientConnection.h"

#include "robomongo/core/settings/ConnectionSettings.h"

namespace{
    mongo::mutex _mutex( "RDBClientConnection::_mutex" );
}

namespace Robomongo
{
    class SSHSocket :public mongo::Socket
    {
        typedef mongo::Socket BaseClass;
    public: 
        SSHSocket(int sock, const mongo::SockAddr& farEnd): BaseClass(sock, farEnd){}
        SSHSocket(double so_timeout = 0, int logLevel = 0 ): BaseClass(so_timeout, logLevel){}

    private:
        virtual int _send( const char * data , int len )
        {
#ifdef MONGO_SSL
            if ( _ssl ) {
                return SSL_write( _ssl , data , len );
            }
#endif
            return ::send( _fd , data , len , 0 );
        }
        virtual int _recv( char * buf , int max )
        {
#ifdef MONGO_SSL
            if ( _ssl ){
                return SSL_read( _ssl , buf , max );
            }
#endif
            return ::recv( _fd , buf , max , 0 );
        }
    };

    RDBClientConnection::RDBClientConnection(bool _autoReconnect, mongo::DBClientReplicaSet* cp, double so_timeout) 
        :BaseClass(_autoReconnect,cp,so_timeout) {

    }

    bool RDBClientConnection::connect(const ConnectionSettings *const connection)
    {
        mongo::scoped_lock lk(_mutex);
        mongo::cmdLine.sslOnNormalPorts = connection->isSslSupport();
        mongo::cmdLine.sslPEMKeyFile = connection->sslPEMKeyFile();

        _server = connection->getFullAddress();
        _serverString = _server.toString();
        _serverString = _server.toString();

        server.reset(new mongo::SockAddr(_server.host().c_str(), _server.port()));
        SSHInfo info = connection->sshInfo();

        if(info.isValid()){
            boost::shared_ptr<mongo::Socket> sock(new SSHSocket( _so_timeout, _logLevel ));
            p.reset(new mongo::MessagingPort(sock));
        }else{
            p.reset(new mongo::MessagingPort( _so_timeout, _logLevel ));
        }

        if (_server.host().empty() || server->getAddr() == "0.0.0.0") {
            return false;
        }

        if ( !p->connect(*server) ) {
            _failed = true;
            return false;
        }

#ifdef MONGO_SSL
        if ( connection->isSslSupport() ) {
            p->secure( sslManager() );
        }
#endif

        return true;
    }
}
