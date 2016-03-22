#pragma once

#include <QObject>
#include "robomongo/core/events/MongoEvents.h"

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;
    class EstablishSshConnectionRequest;

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

    private:
        void reply(QObject *receiver, Event *event);

        QThread *_thread;
        QAtomicInteger<int> _isQuiting;
        ConnectionSettings* _settings;

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
