#include "ConnectionBasicTab.h"
#include <QLabel>

#include "settings/ConnectionSettings.h"
#include <QGridLayout>

using namespace Robomongo;

ConnectionBasicTab::ConnectionBasicTab(ConnectionSettings *settings) :
    _settings(settings)
{
    QLabel *authLabel = new QLabel(
        "Choose any connection name that will help you to identify this connection.");
    authLabel->setWordWrap(true);
    authLabel->setContentsMargins(0, -2, 0, 20);

    QLabel *serverLabel = new QLabel(
        "Specify host and port of MongoDB server. Host can be either IP or domain name.");
    serverLabel->setWordWrap(true);
    serverLabel->setContentsMargins(0, -2, 0, 20);

    _connectionName = new QLineEdit(_settings->connectionName());
    _serverAddress = new QLineEdit(_settings->databaseAddress());
    _serverPort = new QLineEdit(QString::number(_settings->databasePort()));
    _serverPort->setFixedWidth(80);

    _defaultDatabaseName = new QLineEdit();

    QGridLayout *connectionLayout = new QGridLayout;
    connectionLayout->addWidget(new QLabel("Name:"),      1, 0);
    connectionLayout->addWidget(_connectionName,         1, 1, 1, 3);
    connectionLayout->addWidget(authLabel,               2, 1, 1, 3);
    connectionLayout->addWidget(serverLabel, 4, 1, 1, 3);
    connectionLayout->addWidget(new QLabel("Address:"),    3, 0);
    connectionLayout->addWidget(_serverAddress,          3, 1);
    connectionLayout->addWidget(new QLabel(":"),      3, 2);
    connectionLayout->addWidget(_serverPort,             3, 3);
    connectionLayout->setAlignment(Qt::AlignTop);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(connectionLayout);
    setLayout(mainLayout);

    _connectionName->setFocus();
}

void ConnectionBasicTab::accept()
{
    _settings->setConnectionName(_connectionName->text());
    _settings->setDatabaseAddress(_serverAddress->text());
    _settings->setDatabasePort(_serverPort->text().toInt());
}
