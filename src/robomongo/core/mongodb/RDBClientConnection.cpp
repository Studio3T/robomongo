#include "robomongo/core/mongodb/RDBClientConnection.h"

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/Logger.h"
#include "libssh2.h"

namespace{
    mongo::mutex _mutex( "RDBClientConnection::_mutex" );
    static void kbd_callback(const char *name, int name_len,
        const char *instruction, int instruction_len,
        int num_prompts,
        const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
        LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
        void **abstract)
    {
        
    } /* kbd_callback */ 
}

namespace Robomongo
{

#ifdef OPENSSH_SUPPORT_ENABLED
    class SSHSocket :public mongo::Socket
    {
    public: 
        typedef mongo::Socket BaseClass;
        SSHSocket(const SSHInfo &info, int sock, const mongo::SockAddr& farEnd): BaseClass(sock, farEnd),_info(info),_session(NULL),_channel(NULL){}
        SSHSocket(const SSHInfo &info, double so_timeout = 0, int logLevel = 0 ): BaseClass(so_timeout, logLevel),_info(info),_session(NULL),_channel(NULL){}
        virtual bool connect(mongo::SockAddr& farEnd)
        {            
            static int rc = libssh2_init (0);
            if (rc != 0) {
                return false;
            }
            const char *username = _info._userName.c_str();
            const char *password = _info._password.c_str();
            const char *address = _info._hostName.c_str();
            int port = _info._port;

            bool connectToSSh = BaseClass::connect(farEnd);
            if(!connectToSSh)
                return false;

            _session = libssh2_session_init();
            if (libssh2_session_handshake(_session, _fd)) {
                return false;
            }

            int auth_pw = 0;
            const char *fingerprint = libssh2_hostkey_hash(_session, LIBSSH2_HOSTKEY_HASH_SHA1);
            char *userauthlist = libssh2_userauth_list(_session, username, strlen(username));
            if (strstr(userauthlist, "password") != NULL) {
                auth_pw |= 1;
            }
            if (strstr(userauthlist, "keyboard-interactive") != NULL) {
                auth_pw |= 2;
            }
            if (strstr(userauthlist, "publickey") != NULL) {
                auth_pw |= 4;
            }

            if (auth_pw & 1) {
                /* We could authenticate via password */ 
                if (libssh2_userauth_password(_session, username, password)) {
                    LOG_MSG("Authentication by password failed!",mongo::LL_ERROR);
                    return false;
                }
            } else if (auth_pw & 2) {
                /* Or via keyboard-interactive */ 
                if (libssh2_userauth_keyboard_interactive(_session, username, &kbd_callback) ) {
                        LOG_MSG("Authentication by keyboard-interactive failed!",mongo::LL_ERROR);
                        return false;
                }
            } else if (auth_pw & 4) {
                /* Or by public key */ 
                //if (libssh2_userauth_publickey_fromfile(_session, username, keyfile1, keyfile2, password)) 
                {
                        LOG_MSG("Authentication by public key failed!",mongo::LL_ERROR);
                        return false;
                }
            } else {
                LOG_MSG("No supported authentication methods found!",mongo::LL_ERROR);
                return false;
            }
            /* Request a shell */ 
            if (!(_channel = libssh2_channel_direct_tcpip(_session,address,port))) {
                LOG_MSG("Unable to open a session",mongo::LL_ERROR);
                return false;
            }

            return true;
        }
    private:
        virtual int _send( const char * data , int len )
        {
            return libssh2_channel_write(_channel,data,len);
        }
      
        virtual int _recv( char * buf , int max )
        {
            return libssh2_channel_read(_channel,buf,max);
        }

        const SSHInfo _info;
        LIBSSH2_SESSION *_session;
        LIBSSH2_CHANNEL *_channel;
    };
#endif OPENSSH_SUPPORT_ENABLED

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
#ifdef OPENSSH_SUPPORT_ENABLED
        SSHInfo info = connection->sshInfo();

        if(info.isValid()){
            boost::shared_ptr<mongo::Socket> sock(new SSHSocket(info,_so_timeout, _logLevel ));
            p.reset(new mongo::MessagingPort(sock));
        }else
#endif
        {
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
