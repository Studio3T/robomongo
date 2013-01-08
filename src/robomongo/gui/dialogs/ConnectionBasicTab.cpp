#include "ConnectionBasicTab.h"
#include <QLabel>

#include "settings/ConnectionSettings.h"
#include <QGridLayout>

using namespace Robomongo;

ConnectionBasicTab::ConnectionBasicTab(ConnectionSettings *settings) :
    _settings(settings)
{
    QLabel *connectionDescriptionLabel = new QLabel(
        "Choose any connection name that will help you to identify this connection.");
    connectionDescriptionLabel->setWordWrap(true);
    connectionDescriptionLabel->setContentsMargins(0, -2, 0, 20);

    QLabel *serverDescriptionLabel = new QLabel(
        "Specify host and port of MongoDB server. Host can be either IP or domain name.");
    serverDescriptionLabel->setWordWrap(true);
    serverDescriptionLabel->setContentsMargins(0, -2, 0, 20);

    _connectionName = new QLineEdit(_settings->connectionName());
    _serverAddress = new QLineEdit(_settings->serverHost());
    _serverPort = new QLineEdit(QString::number(_settings->serverPort()));
    _serverPort->setFixedWidth(80);

    QGridLayout *connectionLayout = new QGridLayout;
    connectionLayout->addWidget(new QLabel("Name:"),      1, 0);
    connectionLayout->addWidget(_connectionName,         1, 1, 1, 3);
    connectionLayout->addWidget(connectionDescriptionLabel,               2, 1, 1, 3);
    connectionLayout->addWidget(serverDescriptionLabel, 4, 1, 1, 3);
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
    _settings->setServerHost(_serverAddress->text());
    _settings->setServerPort(_serverPort->text().toInt());
}
