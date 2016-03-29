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

    class SshTunnelWorker : public QObject
    {
    Q_OBJECT

    public:
        explicit SshTunnelWorker(ConnectionSettings *settings);
        ~SshTunnelWorker();

        static void logCallbackHandler(rbm_ssh_session* session, char *message, int level);

    protected:
        void stopAndDelete();

    protected Q_SLOTS: // handlers:
        void init();
        void handle(EstablishSshConnectionRequest *event);
        void handle(ListenSshConnectionRequest *event);

    private:
        void reply(QObject *receiver, Event *event);
        void log(const std::string& message, int level = 3);

        QThread *_thread;
        QAtomicInteger<int> _isQuiting;
        ConnectionSettings* _settings;
        rbm_ssh_tunnel_config* _sshConfig;
        rbm_ssh_session* _sshSession;

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
    };
}
