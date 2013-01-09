#pragma once

#include <QDialog>
#include <QThread>
#include <QLabel>
#include <QIcon>

namespace Robomongo
{
    class ConnectionSettings;

    class ConnectionDiagnosticDialog : public QDialog
    {
        Q_OBJECT
    public:
        ConnectionDiagnosticDialog(ConnectionSettings *connection);

    protected slots:
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

    signals:
        void connectionStatus(QString error, bool connected);
        void authStatus(QString error, bool authed);
        void completed();

    protected:
        void run();

    private:
        ConnectionSettings *_connection;
    };
}
