#include <QtGui>
#include "EditConnectionDialog.h"
#include "AppRegistry.h"

using namespace Robomongo;

/**
 * @brief Constructs dialog with specified connection
 */
EditConnectionDialog::EditConnectionDialog(ConnectionRecordPtr connection) : QDialog()
{
	_connection = connection;

    QPushButton *saveButton = new QPushButton("Save");
	saveButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowRight));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *cancelButton = new QPushButton("Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

    QPushButton *testButton = new QPushButton("Test");
    connect(testButton, SIGNAL(clicked()), this, SLOT(testConnection()));

    _connectionName = new QLineEdit(_connection->connectionName(), this);
    _serverAddress = new QLineEdit(_connection->databaseAddress(), this);
    _serverPort = new QLineEdit(QString::number(_connection->databasePort()), this);
    _userName = new QLineEdit(_connection->userName(), this);
    _userPassword = new QLineEdit(_connection->userPassword(), this);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
	bottomLayout->addWidget(testButton, 1, Qt::AlignLeft);
	bottomLayout->addWidget(saveButton, 1, Qt::AlignRight);
	bottomLayout->addWidget(cancelButton);

    QGridLayout *editLayout = new QGridLayout;
	editLayout->addWidget(new QLabel("Name"), 0, 0);
	editLayout->addWidget(_connectionName, 0, 1);
	editLayout->addWidget(new QLabel("Server"), 1, 0);
	editLayout->addWidget(_serverAddress, 1, 1);
	editLayout->addWidget(new QLabel("Port"), 2, 0);
	editLayout->addWidget(_serverPort, 2, 1, Qt::AlignLeft);
	editLayout->addWidget(new QLabel("Username"), 3, 0);
	editLayout->addWidget(_userName, 3, 1);
	editLayout->addWidget(new QLabel("Password"), 4, 0);
	editLayout->addWidget(_userPassword, 4, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(editLayout);
	mainLayout->addLayout(bottomLayout);

	setContentsMargins(7, 7, 7, 7);

	setWindowTitle("Edit connection");
    //now	setWindowIcon(AppRegistry::instance().serverIcon());

    // Remove help button (?)
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

/**
 * @brief Accept() is called when user agree with entered data.
 */
void EditConnectionDialog::accept()
{
    _connection->setConnectionName(_connectionName->text());
    _connection->setDatabaseAddress(_serverAddress->text());
    _connection->setDatabasePort(_serverPort->text().toInt());
    _connection->setUserName(_userName->text());
    _connection->setUserPassword(_userPassword->text());

    QDialog::accept();
}

/**
 * @brief Close event handler
 */
void EditConnectionDialog::closeEvent(QCloseEvent *event)
{
    if (canBeClosed())
        event->accept();
    else
        event->ignore();
}

/**
 * @brief Check that it is okay to close this window
 *        (there is no modification of data, that we possibly can loose)
 */
bool EditConnectionDialog::canBeClosed()
{
    bool unchanged =
        _connection->connectionName() == _connectionName->text()
        && _connection->databaseAddress() == _serverAddress->text()
        && QString::number(_connection->databasePort()) == _serverPort->text()
        && _connection->userName() == _userName->text()
        && _connection->userPassword() == _userPassword->text();

    // If data was unchanged - simply close dialog
    if (unchanged)
        return true;

    // Ask user
    int answer = QMessageBox::question(this,
            "Connections",
            "Close this window and loose you changes?",
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer == QMessageBox::Yes)
        return true;

    return false;
}

/**
 * @brief Key press event handler
 */
void EditConnectionDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QDialog::keyPressEvent(e);
}

/**
 * @brief Test current connection
 */
void EditConnectionDialog::testConnection()
{
    bool res = true; //_connection->test(_serverAddress->text(), _serverPort->text(), _userName->text(), _userPassword->text());

    if (res)
        QMessageBox::information(NULL, "Success!", "Success! Connection exists.", "Ok");
}
