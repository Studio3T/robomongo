#pragma once

#include <QObject>
#include "robomongo/core/events/MongoEvents.h"

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

struct ssh_tunnel_config;
struct ssh_session;

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
        void stopAndDelete();

    protected Q_SLOTS: // handlers:
        void init();
        void handle(EstablishSshConnectionRequest *event);
        void handle(ListenSshConnectionRequest *event);

    private:
        void reply(QObject *receiver, Event *event);

        QThread *_thread;
        QAtomicInteger<int> _isQuiting;
        ConnectionSettings* _settings;
        ssh_tunnel_config* _sshConfig;
        ssh_session* _sshConnection;

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
