#pragma once

#include <QObject>

#include <robomongo/ssh/ssh.h>
#include "robomongo/core/events/MongoEvents.h"

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

struct rbm_ssh_tunnel_config;
struct rbm_ssh_session;

namespace Robomongo
{
    class ConnectionSettings;
    class EstablishSshConnectionRequest;
    class ListenSshConnectionRequest;

    /*
     * Helper class for creating rbm_ssh_tunnel_config from ConnectionSettings
     * Automatically deletes allocated structure
     */
    class SshTunnelConfigCreator {
    public:
        explicit SshTunnelConfigCreator(ConnectionSettings *settings);
        ~SshTunnelConfigCreator();
        rbm_ssh_tunnel_config *config() { return _sshConfig; }

    private:
        std::string _sshhost;
        int _sshport;
        std::string _remotehost;
        int _remoteport;
        std::string _localip;
        int _localport;

        std::string _userName;
        std::string _userPassword;
        std::string _privateKeyFile;
        std::string _publicKeyFile;
        std::string _passphrase;
        std::string _authMethod; // "password" or "publickey"

        rbm_ssh_tunnel_config* _sshConfig;
    };

    class SshTunnelWorker : public QObject
    {
    Q_OBJECT

    public:
        explicit SshTunnelWorker(ConnectionSettings *settings);
        ~SshTunnelWorker();

        static void logCallbackHandler(void* context, char *message, int level);

    protected:
        void stopAndDelete();

    protected Q_SLOTS: // handlers:
        void handle(EstablishSshConnectionRequest *event);
        void handle(ListenSshConnectionRequest *event);

    private:
        void reply(QObject *receiver, Event *event);
        void log(const std::string& message, int level = 3);

        QThread *_thread;
        QAtomicInteger<int> _isQuiting;
        ConnectionSettings* _settings;
        rbm_ssh_session* _sshSession;

        SshTunnelConfigCreator _configCreator;
    };

}
