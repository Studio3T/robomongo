#include "robomongo/gui/dialogs/ConnectionAdvancedTab.h"

#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>

#include <mongo/client/mongo_uri.h>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/utils/GuiConstants.h"

namespace Robomongo
{

    ConnectionAdvancedTab::ConnectionAdvancedTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        QLabel *defaultDatabaseDescriptionLabel = new QLabel(
            "Database, that will be default (<code>db</code> shell variable will point to this database). "
            "By default, default database will be the one you authenticate on, or <code>test</code> otherwise. "
            "Leave this field empty, if you want default behaviour.");
        defaultDatabaseDescriptionLabel->setWordWrap(true);
        defaultDatabaseDescriptionLabel->setContentsMargins(0, -2, 0, 20);

        _defaultDatabaseName = new QLineEdit(QtUtils::toQString(_settings->defaultDatabase()));

        _uriString = new QLineEdit;
        _uriString->setReadOnly(true);

        auto copyButton = new QPushButton("Copy");
        copyButton->setFixedWidth(40);
        copyButton->setMaximumHeight(HighDpiContants::WIN_HIGH_DPI_BUTTON_HEIGHT);
        VERIFY(connect(copyButton, SIGNAL(clicked()), this, SLOT(on_copyButton_clicked())));

        auto mainLayout = new QGridLayout;
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->addWidget(new QLabel("Default Database:"),          1, 0);
        mainLayout->addWidget(_defaultDatabaseName,                     1, 1, 1, 2);
        mainLayout->addWidget(defaultDatabaseDescriptionLabel,          2, 1, 1, 2);
        mainLayout->addWidget(new QLabel{ "URI Connection String:" },   3, 0);
        mainLayout->addWidget(_uriString,                               3, 1);
        mainLayout->addWidget(copyButton,                               3, 2);

        setLayout(mainLayout);
    }

    void ConnectionAdvancedTab::accept()
    {
        _settings->setDefaultDatabase(QtUtils::toStdString(_defaultDatabaseName->text()));
    }

    void ConnectionAdvancedTab::on_copyButton_clicked()
    {
        // todo: 
        //if (_uriString->text().isEmpty()) {
        //    // todo
        //    //QMessageBox::critical(this, tr("Robomongo"), tr("..."));
        //}
        //else {


        if (_settings->isReplicaSet()) {
            // todo: handle setname will be empty initially
            std::string connStr = { "mongodb://" };
            

            if (_settings->hasEnabledPrimaryCredential() && !_settings->credentials().isEmpty()) {
                connStr.append(_settings->primaryCredential()->userName() + ":" +
                               _settings->primaryCredential()->userPassword() + "@");
            }
            
            for (auto const& member : _settings->replicaSetSettings()->membersToHostAndPort()) {
                connStr.append(member.toString() + ",");
            }
            connStr.pop_back(); // delete last ","
            
            connStr.append("/admin?");
            
            connStr.append("replicaSet=" + _settings->replicaSetSettings()->setName());

            // todo: move ssl enabled into settings
            if (_settings->sslSettings()->sslEnabled()) 
                connStr.append("&ssl=true");

            if (_settings->hasEnabledPrimaryCredential() && !_settings->credentials().isEmpty())
                connStr.append("&authSource=" + _settings->primaryCredential()->databaseName());
                        
            // todo: validate connStr
            //auto uriWithStatus = mongo::StatusWith<mongo::MongoURI>(mongo::MongoURI::parse(connStr));
            //auto str2 = uriWithStatus.getValue().toString();

            _uriString->setText(QString::fromStdString(connStr));
            _uriString->setCursorPosition(QTextCursor::Start);

        }
        else { // standalone server
            mongo::HostAndPort server = _settings->hostAndPort();
            mongo::ConnectionString connStr{ server };
            auto str1 = connStr.toString();

            auto uriWithStatus = mongo::StatusWith<mongo::MongoURI>(mongo::MongoURI::parse(connStr.toString()));
            auto str2 = uriWithStatus.getValue().toString();
                
            _uriString->setText(QString::fromStdString("mongodb://" + str1 + "/admin"));
            _uriString->setCursorPosition(QTextCursor::Start);
        }

        QClipboard *clipboard = qApp->clipboard();
        clipboard->setText(_uriString->text());

        //QMessageBox::information(this, tr("Robomongo"), tr("Copied into clipboard."));
    }

}

