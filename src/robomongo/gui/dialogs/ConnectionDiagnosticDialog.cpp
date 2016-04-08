#include "robomongo/gui/dialogs/ConnectionDiagnosticDialog.h"

#include <QGridLayout>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QMovie>
#include <QMessageBox>

#include <boost/scoped_ptr.hpp>
#include <mongo/client/dbclientinterface.h>
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/mongodb/SshTunnelWorker.h"

namespace Robomongo
{
    ConnectionDiagnosticDialog::ConnectionDiagnosticDialog(ConnectionSettings *connection, QWidget *parent) :
        QDialog(parent),
        _connection(NULL),
        _server(NULL)
    {
        _connection = connection->clone();

        AppRegistry::instance().bus()->subscribe(this, ConnectionEstablishedEvent::Type);
        AppRegistry::instance().bus()->subscribe(this, ConnectionFailedEvent::Type);

        setWindowTitle("Diagnostic");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        //setFixedSize(dialogSize);

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

        _loadingMovie = new QMovie(":robomongo/icons/loading_ticks_40x40.gif", QByteArray(), this);
        _loadingMovie->setScaledSize(QSize(20, 20));
        _loadingMovie->start();

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
        layout->setSizeConstraint(QLayout::SetFixedSize);

        QVBoxLayout *box = new QVBoxLayout;
        box->addLayout(layout);
        box->addSpacing(10);
        box->addWidget(closeButton, 0, Qt::AlignRight);
        setLayout(box);

        sshStatus(InitialState);
        connectionStatus(InitialState);
        authStatus(InitialState);
        listStatus(InitialState);

        // Fit size of dialog to content
        box->activate();
        setFixedSize(minimumSizeHint());

        _server = AppRegistry::instance().app()->openServer(_connection, ConnectionTest);
    }

    ConnectionDiagnosticDialog::~ConnectionDiagnosticDialog() {
        if (_server)
            AppRegistry::instance().app()->closeServer(_server);
    }

    void ConnectionDiagnosticDialog::sshStatus(State state)
    {
        if (!_connection->sshSettings()->enabled()) {
            _sshIconLabel->setVisible(false);
            _sshLabel->setVisible(false);
            return;
        }

        if (state == InitialState) {
            _sshIconLabel->setMovie(_loadingMovie);
            _sshLabel->setText(QString("Connecting to SSH server at <b>%1:%2</b>...")
                .arg(QtUtils::toQString(_connection->sshSettings()->host()))
                .arg(_connection->sshSettings()->port()));
        } else if (state == CompletedState) {
            _sshIconLabel->setPixmap(_yesPixmap);
            _sshLabel->setText(QString("Connected to SSH server at <b>%1:%2</b>")
                .arg(QtUtils::toQString(_connection->sshSettings()->host()))
                .arg(_connection->sshSettings()->port()));
        } else if (state == FailedState) {
            _sshIconLabel->setPixmap(_noPixmap);
            _sshLabel->setText(QString("Unable to connect to SSH server at <b>%1:%2</b>")
                .arg(QtUtils::toQString(_connection->sshSettings()->host()))
                .arg(_connection->sshSettings()->port()));
        }

        layout()->activate();
        setFixedSize(minimumSizeHint());
    }

    void ConnectionDiagnosticDialog::connectionStatus(State state)
    {
        if (state == InitialState) {
            _connectionIconLabel->setMovie(_loadingMovie);
            _connectionLabel->setText(QString("Connecting to <b>%1</b>...") .arg(QtUtils::toQString(_connection->getFullAddress())));
        } else if (state == CompletedState) {
            _connectionIconLabel->setPixmap(_yesPixmap);
            _connectionLabel->setText(QString("Connected to <b>%1</b>").arg(QtUtils::toQString(_connection->getFullAddress())));
        } else if (state == FailedState) {
            _connectionIconLabel->setPixmap(_noPixmap);
            _connectionLabel->setText(QString("Unable to connect to <b>%1</b>").arg(QtUtils::toQString(_connection->getFullAddress())));
        } else if (state == NotPerformedState) {
            _connectionIconLabel->setPixmap(_questionPixmap);
            _connectionLabel->setText(QString("No chance to try connection to <b>%1</b>").arg(QtUtils::toQString(_connection->getFullAddress())));
        }

        layout()->activate();
        setFixedSize(minimumSizeHint());
    }

    void ConnectionDiagnosticDialog::authStatus(State state)
    {
        if (!_connection->hasEnabledPrimaryCredential()) {
            _authIconLabel->setVisible(false);
            _authLabel->setVisible(false);
            return;
        }

        if (state == InitialState) {
            _authIconLabel->setMovie(_loadingMovie);
            _authLabel->setText(QString("Authorizing on <b>%1</b> database as <b>%2</b>...")
                .arg(QtUtils::toQString(_connection->primaryCredential()->databaseName()))
                .arg(QtUtils::toQString(_connection->primaryCredential()->userName())));
        } else if (state == CompletedState) {
            _authIconLabel->setPixmap(_yesPixmap);
            _authLabel->setText(QString("Authorized on <b>%1</b> database as <b>%2</b>")
                .arg(QtUtils::toQString(_connection->primaryCredential()->databaseName()))
                .arg(QtUtils::toQString(_connection->primaryCredential()->userName())));
        } else if (state == FailedState) {
            _authIconLabel->setPixmap(_noPixmap);
            _authLabel->setText(QString("Authorization failed on <b>%1</b> database as <b>%2</b>")
                .arg(QtUtils::toQString(_connection->primaryCredential()->databaseName()))
                .arg(QtUtils::toQString(_connection->primaryCredential()->userName())));
        } else if (state == NotPerformedState) {
            _authIconLabel->setPixmap(_questionPixmap);
            _authLabel->setText(QString("No chance to authorize"));
        }

        layout()->activate();
        setFixedSize(minimumSizeHint());
    }

    void ConnectionDiagnosticDialog::listStatus(State state)
    {
        if (_connection->hasEnabledPrimaryCredential()) {
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

        layout()->activate();
        setFixedSize(minimumSizeHint());
    }

    void ConnectionDiagnosticDialog::handle(ConnectionEstablishedEvent *event) {
        sshStatus(CompletedState);
        connectionStatus(CompletedState);
        authStatus(CompletedState);
        listStatus(CompletedState);
    }

    void ConnectionDiagnosticDialog::handle(ConnectionFailedEvent *event) {
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
        }
    }
}
