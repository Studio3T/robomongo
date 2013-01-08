#include "ConnectionAdvancedTab.h"
#include <QLabel>
#include <QGridLayout>
#include "settings/ConnectionSettings.h"

using namespace Robomongo;

ConnectionAdvancedTab::ConnectionAdvancedTab(ConnectionSettings *settings) :
    _settings(settings)
{
    QLabel *defaultDatabaseDescriptionLabel = new QLabel(
        "Database, that will be default (<code>db</code> shell variable will point to this database). "
        "By default, default database will be the one you authenticate on, or <code>test</code> otherwise. "
        "Leave this field empty, if you want default behaviour.");
    defaultDatabaseDescriptionLabel->setWordWrap(true);
    defaultDatabaseDescriptionLabel->setContentsMargins(0, -2, 0, 20);

    _defaultDatabaseName = new QLineEdit(_settings->defaultDatabase());

    QGridLayout *connectionLayout = new QGridLayout;
    connectionLayout->setAlignment(Qt::AlignTop);
    connectionLayout->addWidget(new QLabel("Default Database:"),  1, 0);
    connectionLayout->addWidget(_defaultDatabaseName,             1, 1, 1, 1);
    connectionLayout->addWidget(defaultDatabaseDescriptionLabel,  2, 1, 1, 1);

    setLayout(connectionLayout);
}

void ConnectionAdvancedTab::accept()
{
    _settings->setDefaultDatabase(_defaultDatabaseName->text());
}
