#include "robomongo/gui/dialogs/ConnectionDiagnosticDialog.h"

#include <QGridLayout>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QMovie>

#include <mongo/client/dbclientinterface.h>
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"

using namespace mongo;
namespace Robomongo
{
    ConnectionDiagnosticDialog::ConnectionDiagnosticDialog(ConnectionSettings *connection) :
        _connection(connection),
        _connectionStatusReceived(false),
        _authStatusReceived(false)
    {
        setWindowTitle("Diagnostic");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)

        _yesIcon = GuiRegistry::instance().yesMarkIcon();
        _noIcon = GuiRegistry::instance().noMarkIcon();
        _yesPixmap = _yesIcon.pixmap(24, 24);
        _noPixmap = _noIcon.pixmap(24, 24);

        QPushButton *closeButton = new QPushButton("&Close");
        VERIFY(connect(closeButton, SIGNAL(clicked()), this, SLOT(accept())));
        _connectionIconLabel = new QLabel;
        _authIconLabel = new QLabel;
        _connectionLabel = new QLabel;
        _authLabel = new QLabel;

        _loadingMovie = new QMovie(":robomongo/icons/loading.gif", QByteArray(), this);
        _loadingMovie->start();

        _connectionIconLabel->setMovie(_loadingMovie);
        _authIconLabel->setMovie(_loadingMovie);

        _connectionLabel->setText(QString("Connecting to <b>%1</b>...").arg(QtUtils::toQString(_connection->getFullAddress())));

        if (_connection->hasEnabledPrimaryCredential()) {
            _authLabel->setText(QString("Authorizing on <b>%1</b> database as <b>%2</b>...")
                .arg(QtUtils::toQString(_connection->primaryCredential()->databaseName()))
                .arg(QtUtils::toQString(_connection->primaryCredential()->userName())));
        } else {
            _authLabel->setText("Authorization skipped by you");
        }

        QGridLayout *layout = new QGridLayout();
        layout->setContentsMargins(20, 20, 20, 10);
        layout->addWidget(_connectionIconLabel, 0, 0);
        layout->addWidget(_connectionLabel,     0, 1, Qt::AlignLeft);
        layout->addWidget(_authIconLabel,       1, 0);
        layout->addWidget(_authLabel,           1, 1, Qt::AlignLeft);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        QVBoxLayout *box = new QVBoxLayout;
        box->addLayout(layout);
        box->addSpacing(10);
        box->addWidget(closeButton, 0, Qt::AlignRight);
        setLayout(box);

        ConnectionDiagnosticThread *thread = new ConnectionDiagnosticThread(_connection);
        VERIFY(connect(thread, SIGNAL(connectionStatus(QString, bool)), this, SLOT(connectionStatus(QString, bool))));
        VERIFY(connect(thread, SIGNAL(authStatus(QString, bool)), this, SLOT(authStatus(QString, bool))));
        VERIFY(connect(thread, SIGNAL(completed()), this, SLOT(completed())));
        VERIFY(connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater())));
        thread->start();
    }

    void ConnectionDiagnosticDialog::connectionStatus(QString error, bool connected)
    {
        _connectionStatusReceived = true;

        if (connected) {
            _connectionIconLabel->setPixmap(_yesPixmap);
            _connectionLabel->setText(QString("Connected to <b>%1</b>").arg(QtUtils::toQString(_connection->getFullAddress())));
        } else {
            _connectionIconLabel->setPixmap(_noPixmap);
            _connectionLabel->setText(QString("Unable to connect to <b>%1</b>").arg(QtUtils::toQString(_connection->getFullAddress())));
        }

        layout()->activate();
        setFixedSize(minimumSizeHint());
    }

    void ConnectionDiagnosticDialog::authStatus(QString error, bool authed)
    {
        _authStatusReceived = true;

        if (authed) {
            _authIconLabel->setPixmap(_yesPixmap);
            _authLabel->setText(QString("Authorized as <b>%1</b>").arg(QtUtils::toQString(_connection->primaryCredential()->userName())));
        } else {

            _authIconLabel->setPixmap(_noPixmap);

            if (_connection->hasEnabledPrimaryCredential())
                _authLabel->setText(QString("Authorization failed"));
            else
                _authLabel->setText(QString("Authorization skipped by you"));
        }

        layout()->activate();
        setFixedSize(minimumSizeHint());
    }

    void ConnectionDiagnosticDialog::completed()
    {
        if (!_connectionStatusReceived)
            connectionStatus("", false);

        if (!_authStatusReceived)
            authStatus("", false);

        // seems that it is wrong to call any method on thread,
        // because thread already can be disposed.
        // QThread *thread = static_cast<QThread *>(sender());
        // thread->quit();
    }


    ConnectionDiagnosticThread::ConnectionDiagnosticThread(ConnectionSettings *connection) :
        _connection(connection) { }

    void ConnectionDiagnosticThread::run()
    {
        char address[256]={0};
        sprintf(address,"%s:%d",_connection->serverHost().c_str(),_connection->serverPort());

        boost::scoped_ptr<DBClientConnection> connection;

        try {
            connection.reset(new DBClientConnection);
            connection->connect(address);
            emit connectionStatus("", true);
        }
        catch(UserException &ex) {
            const char *what = ex.what();
            emit connectionStatus(QString(what), false);
            emit completed();
            return;
        }

        try {
            if (_connection->hasEnabledPrimaryCredential())
            {
                CredentialSettings *credential = _connection->primaryCredential();
                std::string database = credential->databaseName();
                std::string username = credential->userName();
                std::string password = credential->userPassword();

                string errmsg;
                bool ok = connection->auth(database, username, password, errmsg);
                if (!ok) {
                    emit authStatus("", false);
                } else {
                    emit authStatus("", true);
                }
            }
        } catch (const UserException &) {
            emit authStatus("", false);
        }

        emit completed();
    }
}
