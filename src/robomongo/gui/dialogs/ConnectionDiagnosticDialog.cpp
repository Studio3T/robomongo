#include "robomongo/gui/dialogs/ConnectionDiagnosticDialog.h"

#include <QGridLayout>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QMovie>
#include <QMessageBox>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/mongodb/SshTunnelWorker.h"

namespace Robomongo
{
    ConnectionDiagnosticDialog::ConnectionDiagnosticDialog(ConnectionSettings *connection, QWidget *parent) :
        QDialog(parent),
        _connSettings(connection->clone()),
        _server(NULL),
        _serverHandle(0),
        _continueExec(true)
    {
        AppRegistry::instance().bus()->subscribe(this, ConnectionEstablishedEvent::Type);
        AppRegistry::instance().bus()->subscribe(this, ConnectionFailedEvent::Type);

        setWindowTitle("Diagnostic");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)

        _yesIcon = GuiRegistry::instance().yesMarkIcon();
        _noIcon = GuiRegistry::instance().noMarkIcon();
        _questionIcon = GuiRegistry::instance().questionMarkIcon();
        _yesPixmap = _yesIcon.pixmap(20, 20);
        _noPixmap = _noIcon.pixmap(20, 20);
        _questionPixmap = _questionIcon.pixmap(20, 20);

        QPushButton *closeButton = new QPushButton("&Close");
        VERIFY(connect(closeButton, SIGNAL(clicked()), this, SLOT(accept())));
        _connectionIconLabel = new QLabel;
        _connectionLabel = new QLabel;
        _authIconLabel = new QLabel;
        _authLabel = new QLabel;
        _sshIconLabel = new QLabel;
        _sshLabel = new QLabel;
        _listIconLabel = new QLabel;
        _listLabel = new QLabel;

        _viewErrorLink = new QLabel("<a href='error' style='color: #777777;'>Show error details</a>");
        VERIFY(connect(_viewErrorLink, SIGNAL(linkActivated(QString)), this, SLOT(errorLinkActivated(QString))));

        _loadingMovie = new QMovie(":robomongo/icons/loading_ticks_40x40.gif", QByteArray(), this);
        _loadingMovie->setScaledSize(QSize(20, 20));
        _loadingMovie->start();

        _connectionIconLabel->setMovie(_loadingMovie);
        _sshIconLabel->setMovie(_loadingMovie);
        _authIconLabel->setMovie(_loadingMovie);
        _listIconLabel->setMovie(_loadingMovie);

        QGridLayout *layout = new QGridLayout();
        layout->setContentsMargins(20, 20, 20, 10);
        layout->addWidget(_sshIconLabel,        0, 0);
        layout->addWidget(_sshLabel,            0, 1, Qt::AlignLeft);
        layout->addWidget(_connectionIconLabel, 1, 0);
        layout->addWidget(_connectionLabel,     1, 1, Qt::AlignLeft);
        layout->addWidget(_authIconLabel,       2, 0);
        layout->addWidget(_authLabel,           2, 1, Qt::AlignLeft);
        layout->addWidget(_listIconLabel,       3, 0);
        layout->addWidget(_listLabel,           3, 1, Qt::AlignLeft);
        layout->setColumnStretch(0, 0) ; // Give column 0 no stretch ability
        layout->setColumnStretch(1, 1) ; // Give column 1 stretch ability of ratio 1

        QHBoxLayout *hbox = new QHBoxLayout;
        hbox->addSpacing(21);
        hbox->addWidget(_viewErrorLink, 1, Qt::AlignLeft);
        hbox->addWidget(closeButton, 0, Qt::AlignRight);

        QVBoxLayout *box = new QVBoxLayout;
        box->addLayout(layout);
        box->addSpacing(10);
        box->addLayout(hbox);
        setLayout(box);

        sshStatus(InitialState);
        connectionStatus(InitialState);
        authStatus(InitialState);
        listStatus(InitialState);

        _viewErrorLink->hide();

        if (!AppRegistry::instance().app()->openServer(_connSettings, ConnectionTest)) {
            _continueExec = false;
            return;
        }

        _serverHandle = AppRegistry::instance().app()->getLastServerHandle();
    }

    ConnectionDiagnosticDialog::~ConnectionDiagnosticDialog() {
        if (_server)
            AppRegistry::instance().app()->closeServer(_server);
    }

    void ConnectionDiagnosticDialog::errorLinkActivated(const QString &link) {
        if (_lastErrorMessage.empty())
            return;

        QMessageBox::information(this, "Error details", QtUtils::toQString(_lastErrorMessage));
    }

    void ConnectionDiagnosticDialog::sshStatus(State state)
    {
        if (!_connSettings->sshSettings()->enabled()) {
            _sshIconLabel->setVisible(false);
            _sshLabel->setVisible(false);
            return;
        }

        if (state == InitialState) {
            _sshIconLabel->setMovie(_loadingMovie);
            _sshLabel->setText(QString("Connecting to SSH server at <b>%1:%2</b>...")
                .arg(QtUtils::toQString(_connSettings->sshSettings()->host()))
                .arg(_connSettings->sshSettings()->port()));
        } else if (state == CompletedState) {
            _sshIconLabel->setPixmap(_yesPixmap);
            _sshLabel->setText(QString("Connected to SSH server at <b>%1:%2</b>")
                .arg(QtUtils::toQString(_connSettings->sshSettings()->host()))
                .arg(_connSettings->sshSettings()->port()));
        } else if (state == FailedState) {
            _sshIconLabel->setPixmap(_noPixmap);
            _sshLabel->setText(QString("Unable to connect to SSH server at <b>%1:%2</b>")
                .arg(QtUtils::toQString(_connSettings->sshSettings()->host()))
                .arg(_connSettings->sshSettings()->port()));
        }
    }

    void ConnectionDiagnosticDialog::connectionStatus(State state)
    {
        // Add info about tunneling if SSH or SSL is used
        QString tunnelNote("");    // No tunnel info when neither SSH nor SSL enabled
        if (_connSettings->sshSettings()->enabled()) 
            tunnelNote = " via SSH tunnel";
        else if (_connSettings->sslSettings()->sslEnabled())
            tunnelNote = " via SSL tunnel";
        else
            tunnelNote = "";
         

        auto replicaSetStr = QString::fromStdString(_connSettings->connectionName()) + " [Replica Set]";
        replicaSetStr = (_connSettings->replicaSetSettings()->members().size() > 0) 
                        ? replicaSetStr + '(' + QString::fromStdString(
                                                    _connSettings->replicaSetSettings()->members()[0]) + ')'
                        : replicaSetStr + "";

        QString const& serverAddress = _connSettings->isReplicaSet()
                                       ? replicaSetStr
                                       : QString::fromStdString(_connSettings->getFullAddress());

        // Set main info text at dialog
        if (state == InitialState) {
            _connectionIconLabel->setMovie(_loadingMovie);
            _connectionLabel->setText(QString("Connecting to <b>%1</b>%2...").arg(serverAddress, tunnelNote));

        } else if (state == CompletedState) {
            _connectionIconLabel->setPixmap(_yesPixmap);
            _connectionLabel->setText(QString("Connected to <b>%1</b>%2").arg(serverAddress, tunnelNote));
        } else if (state == FailedState) {
            _connectionIconLabel->setPixmap(_noPixmap);
            _connectionLabel->setText(QString("Failed to connect to <b>%1</b>%2").arg(serverAddress, tunnelNote));
        } else if (state == NotPerformedState) {
            _connectionIconLabel->setPixmap(_questionPixmap);
            _connectionLabel->setText(QString("No chance to try connection to <b>%1</b>%2").
                                               arg(serverAddress, tunnelNote));
        }
    }

    void ConnectionDiagnosticDialog::authStatus(State state)
    {
        if (!_connSettings->hasEnabledPrimaryCredential()) {
            _authIconLabel->setVisible(false);
            _authLabel->setVisible(false);
            return;
        }

        if (state == InitialState) {
            _authIconLabel->setMovie(_loadingMovie);
            _authLabel->setText(QString("Authorizing on <b>%1</b> database as <b>%2</b>...")
                .arg(QtUtils::toQString(_connSettings->primaryCredential()->databaseName()))
                .arg(QtUtils::toQString(_connSettings->primaryCredential()->userName())));
        } else if (state == CompletedState) {
            _authIconLabel->setPixmap(_yesPixmap);
            _authLabel->setText(QString("Authorized on <b>%1</b> database as <b>%2</b>")
                .arg(QtUtils::toQString(_connSettings->primaryCredential()->databaseName()))
                .arg(QtUtils::toQString(_connSettings->primaryCredential()->userName())));
        } else if (state == FailedState) {
            _authIconLabel->setPixmap(_noPixmap);
            _authLabel->setText(QString("Authorization failed on <b>%1</b> database as <b>%2</b>")
                .arg(QtUtils::toQString(_connSettings->primaryCredential()->databaseName()))
                .arg(QtUtils::toQString(_connSettings->primaryCredential()->userName())));
        } else if (state == NotPerformedState) {
            _authIconLabel->setPixmap(_questionPixmap);
            _authLabel->setText(QString("No chance to authorize"));
        }
    }

    void ConnectionDiagnosticDialog::listStatus(State state)
    {
        if (_connSettings->hasEnabledPrimaryCredential()) {
            _listIconLabel->setVisible(false);
            _listLabel->setVisible(false);
            return;
        }

        if (state == InitialState) {
            _listIconLabel->setMovie(_loadingMovie);
            _listLabel->setText(QString("Loading list of databases..."));
        } else if (state == CompletedState) {
            _listIconLabel->setPixmap(_yesPixmap);
            _listLabel->setText(QString("Access to databases is available"));
        } else if (state == FailedState) {
            _listIconLabel->setPixmap(_noPixmap);
            _listLabel->setText(QString("Failed to load list of databases"));
        } else if (state == NotPerformedState) {
            _listIconLabel->setPixmap(_questionPixmap);
            _listLabel->setText(QString("No chance to load list of databases"));
        }
    }

    void ConnectionDiagnosticDialog::handle(ConnectionEstablishedEvent *event) {
        if (event->connectionType != ConnectionTest)
            return;

        sshStatus(CompletedState);
        connectionStatus(CompletedState);
        authStatus(CompletedState);
        listStatus(CompletedState);

        // Remember in order to delete on dialog close
        _server = static_cast<MongoServer*>(event->sender());
    }

    void ConnectionDiagnosticDialog::handle(ConnectionFailedEvent *event) {
        if (event->connectionType != ConnectionTest || event->serverHandle != _serverHandle)
            return;

        sshStatus(CompletedState);
        connectionStatus(CompletedState);
        authStatus(CompletedState);
        listStatus(CompletedState);

        switch (event->reason) {
        case ConnectionFailedEvent::SshConnection:
            sshStatus(FailedState);
            connectionStatus(NotPerformedState);
            authStatus(NotPerformedState);
            listStatus(NotPerformedState);
            break;
        case ConnectionFailedEvent::SshChannel:
        case ConnectionFailedEvent::MongoConnection:
            connectionStatus(FailedState);
            authStatus(NotPerformedState);
            listStatus(NotPerformedState);
            break;
        case ConnectionFailedEvent::MongoAuth:
            authStatus(FailedState);
            listStatus(FailedState);
            break;
        case ConnectionFailedEvent::SslConnection:
            connectionStatus(FailedState);
            authStatus(NotPerformedState);
            listStatus(NotPerformedState);
            break;
        }

        // Show link for additional error details
        if (!event->message.empty()) {
            _lastErrorMessage = event->message;
            _viewErrorLink->show();
        }
    }
}
