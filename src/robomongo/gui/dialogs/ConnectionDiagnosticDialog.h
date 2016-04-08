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
    class ConnectionEstablishedEvent;
    class ConnectionFailedEvent;
    class ConnectionSettings;
    class MongoServer;

    class ConnectionDiagnosticDialog : public QDialog
    {
        Q_OBJECT
    public:
        ConnectionDiagnosticDialog(ConnectionSettings *connection, QWidget *parent = 0);
        ~ConnectionDiagnosticDialog();

    protected Q_SLOTS:
        void handle(ConnectionEstablishedEvent *event);
        void handle(ConnectionFailedEvent *event);

    private:

        enum State {
            InitialState,
            CompletedState,
            FailedState,
            NotPerformedState
        };

        void sshStatus(State connected);
        void connectionStatus(State connected);
        void authStatus(State authed);
        void listStatus(State authed);

        ConnectionSettings *_connection;
        QIcon _yesIcon;
        QIcon _noIcon;
        QIcon _questionIcon;
        QMovie *_loadingMovie;

        QPixmap _yesPixmap;
        QPixmap _noPixmap;
        QPixmap _questionPixmap;

        QLabel *_sshIconLabel;
        QLabel *_sshLabel;
        QLabel *_connectionIconLabel;
        QLabel *_connectionLabel;
        QLabel *_authIconLabel;
        QLabel *_authLabel;
        QLabel *_listIconLabel;  // List database names
        QLabel *_listLabel;

        MongoServer *_server;
    };
}
