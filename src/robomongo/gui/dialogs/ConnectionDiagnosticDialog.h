#pragma once

#include <QDialog>
#include <QIcon>

class QLabel;
class QMovie;

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

        void sshStatus(State state);
        void connectionStatus(State state);
        void authStatus(State state);
        void listStatus(State state);

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
