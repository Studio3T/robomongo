#include "ConnectionAdvancedTab.h"
#include <QLabel>
#include <QGridLayout>
#include "settings/ConnectionSettings.h"

using namespace Robomongo;

ConnectionAdvancedTab::ConnectionAdvancedTab(ConnectionSettings *settings) :
    _settings(settings)
{
    QLabel *authLabel = new QLabel(
        "Database, that will be default (<code>db</code> shell variable will point to this database).");
    authLabel->setWordWrap(true);
    authLabel->setContentsMargins(0, -2, 0, 20);

    _defaultDatabaseName = new QLineEdit(_settings->defaultDatabase());

    QGridLayout *connectionLayout = new QGridLayout;
    connectionLayout->setAlignment(Qt::AlignTop);
    connectionLayout->addWidget(new QLabel("Default Database:"),  1, 0);
    connectionLayout->addWidget(_defaultDatabaseName,             1, 1, 1, 1);
    connectionLayout->addWidget(authLabel,                        2, 1, 1, 1);

    setLayout(connectionLayout);

}

void ConnectionAdvancedTab::accept()
{
    _settings->setDefaultDatabase(_defaultDatabaseName->text());
}
