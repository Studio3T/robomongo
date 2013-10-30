#include "robomongo/gui/dialogs/ConnectionBasicTab.h"

#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"

namespace Robomongo
{
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

        _connectionName = new QLineEdit(QtUtils::toQString(_settings->connectionName()));
        _serverAddress = new QLineEdit(QtUtils::toQString(_settings->serverHost()));
        _serverPort = new QLineEdit(QString::number(_settings->serverPort()));
        _serverPort->setFixedWidth(80);
        QRegExp rx("\\d+");//(0-65554)
        _serverPort->setValidator(new QRegExpValidator(rx, this));

        QGridLayout *connectionLayout = new QGridLayout;
        connectionLayout->setAlignment(Qt::AlignTop);
        connectionLayout->addWidget(new QLabel("Name:"),          1, 0);
        connectionLayout->addWidget(_connectionName,              1, 1, 1, 3);
        connectionLayout->addWidget(connectionDescriptionLabel,   2, 1, 1, 3);
        connectionLayout->addWidget(new QLabel("Address:"),       3, 0);
        connectionLayout->addWidget(_serverAddress,               3, 1);
        connectionLayout->addWidget(new QLabel(":"),              3, 2);
        connectionLayout->addWidget(_serverPort,                  3, 3);
        connectionLayout->addWidget(serverDescriptionLabel,       4, 1, 1, 3);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addLayout(connectionLayout);
        setLayout(mainLayout);

        _connectionName->setFocus();
    }

    void ConnectionBasicTab::accept()
    {
        _settings->setConnectionName(QtUtils::toStdString(_connectionName->text()));
        _settings->setServerHost(QtUtils::toStdString(_serverAddress->text()));
        _settings->setServerPort(_serverPort->text().toInt());
    }
}
