#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;
    class SshTunnelWorker : public QObject
    {
    Q_OBJECT

    public:
        explicit SshTunnelWorker(ConnectionSettings *settings);
        ~SshTunnelWorker();
        void stopAndDelete();

    protected Q_SLOTS: // handlers:
        void init();

    private:
        QThread *_thread;
        QAtomicInteger<int> _isQuiting;
        ConnectionSettings* _settings;
    };
}
