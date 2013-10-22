#pragma once

#include <QDialog>
#include <QIcon>
#include <QThread>
QT_BEGIN_NAMESPACE
class QLabel;
class QMovie;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;

    class ConnectionDiagnosticDialog : public QDialog
    {
        Q_OBJECT
    public:
        typedef QDialog BaseClass;
        static const QSize dialogSize;        
        ConnectionDiagnosticDialog(ConnectionSettings *connection, QWidget *parent = 0);

    protected Q_SLOTS:
        void connectionStatus(QString error, bool connected);
        void authStatus(QString error, bool authed);
        void completed();

    private:
        ConnectionSettings *_connection;
        QIcon _yesIcon;
        QIcon _noIcon;
        QIcon _waitIcon;
        QMovie *_loadingMovie;

        QPixmap _yesPixmap;
        QPixmap _noPixmap;

        QLabel *_connectionIconLabel;
        QLabel *_connectionLabel;
        QLabel *_authIconLabel;
        QLabel *_authLabel;

        bool _connectionStatusReceived;
        bool _authStatusReceived;
    };

    class ConnectionDiagnosticThread : public QThread
    {
        Q_OBJECT
    public:
        ConnectionDiagnosticThread(ConnectionSettings *connection);

    Q_SIGNALS:
        void connectionStatus(QString error, bool connected);
        void authStatus(QString error, bool authed);
        void completed();

    protected:
        void run();

    private:
        ConnectionSettings *_connection;
    };
}
