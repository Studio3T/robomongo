#include "robomongo/gui/dialogs/ConnectionAdvancedTab.h"

#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>
/* --- Disabling unfinished export URI connection string feature 
#include <QPushButton>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QCheckBox>
#include <QToolTip>
*/

#include <mongo/client/mongo_uri.h>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/QtUtils.h"
/* --- Disabling unfinished export URI connection string feature 
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/gui/utils/GuiConstants.h"
*/

namespace Robomongo
{

    ConnectionAdvancedTab::ConnectionAdvancedTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        QLabel *defaultDatabaseDescriptionLabel = new QLabel(
            "Database, that will be default (<code>db</code> shell variable will point to this database). "
            "By default, default database will be the one you authenticate on, or <code>test</code> otherwise. "
            "Leave this field empty, if you want default behavior.");
        defaultDatabaseDescriptionLabel->setWordWrap(true);
        defaultDatabaseDescriptionLabel->setContentsMargins(0, -2, 0, 20);

        _defaultDatabaseName = new QLineEdit(QtUtils::toQString(_settings->defaultDatabase()));

        /* --- Disabling unfinished export URI connection string feature
        _uriString = new QLineEdit;
        _uriString->setReadOnly(true);

        _includePasswordCheckBox = new QCheckBox("Include passwords");
        VERIFY(connect(_includePasswordCheckBox, SIGNAL(toggled(bool)), 
                       this, SLOT(on_includePasswordsCheckBox_toggle(bool))));

        auto generateButton = new QPushButton("Generate");
        generateButton->setFixedWidth(70);
        generateButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
        VERIFY(connect(generateButton, SIGNAL(clicked()), this, SLOT(on_generateButton_clicked())));

        _copyButton = new QPushButton("Copy");
        _copyButton->setFixedWidth(50);
        _copyButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
        VERIFY(connect(_copyButton, SIGNAL(clicked()), this, SLOT(on_copyButton_clicked())));

        auto hlay = new QHBoxLayout;
        hlay->addWidget(_includePasswordCheckBox);
        hlay->addWidget(generateButton, Qt::AlignLeft);
        hlay->addWidget(_copyButton, Qt::AlignLeft);
        */

        auto mainLayout = new QGridLayout;
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->addWidget(new QLabel("Default Database:"),          1, 0);
        mainLayout->addWidget(_defaultDatabaseName,                     1, 1, 1, 2);
        mainLayout->addWidget(defaultDatabaseDescriptionLabel,          2, 1, 1, 2);
        /* --- Disabling unfinished export URI connection string feature
        mainLayout->addWidget(new QLabel{ "URI Connection String:" },   3, 0);
        mainLayout->addWidget(_uriString,                               3, 1);
        mainLayout->addLayout(hlay,                                     4, 1);
        */
        setLayout(mainLayout);
    }

    void ConnectionAdvancedTab::accept()
    {
        _settings->setDefaultDatabase(QtUtils::toStdString(_defaultDatabaseName->text()));
    }

    /* --- Disabling unfinished export URI connection string feature
    void ConnectionAdvancedTab::on_generateButton_clicked()
    {
        if (_settings->isReplicaSet()) {
            // todo: 
            // handle set name will be empty initially. It will be available(cached) after first
            // successful connection.
            std::string connStr = { "mongodb://" };

            // todo: which credential to use if there are more than one?
            if (_settings->hasEnabledPrimaryCredential() && !_settings->credentials().isEmpty()) {
                connStr.append(_settings->primaryCredential()->userName() + ":");
                connStr.append(_includePasswordCheckBox->isChecked() ? 
                    _settings->primaryCredential()->userPassword() : "PASSWORD");
                connStr.append("@");
            }

            for (auto const& member : _settings->replicaSetSettings()->membersToHostAndPort()) {
                connStr.append(member.toString() + ",");
            }
            connStr.pop_back(); // delete last ","

            connStr.append("/admin?");

            connStr.append("replicaSet=" + _settings->replicaSetSettings()->setName());

            if (_settings->sslSettings()->sslEnabled())
                connStr.append("&ssl=true");

            if (_settings->hasEnabledPrimaryCredential() && !_settings->credentials().isEmpty())
                connStr.append("&authSource=" + _settings->primaryCredential()->databaseName());

            // todo: validate connStr
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
    }

    void ConnectionAdvancedTab::on_copyButton_clicked()
    {
        if (_uriString->text().isEmpty())
            return;

        QClipboard *clipboard = qApp->clipboard();
        clipboard->setText(_uriString->text());

        auto posUnderGenerateButton = QPoint(_copyButton->pos().x()-100, _copyButton->y()+20);
        QToolTip::showText(mapToGlobal(posUnderGenerateButton), "Copied into clipboard", nullptr, QRect(), 2000);
    }

    void ConnectionAdvancedTab::on_includePasswordsCheckBox_toggle(bool checked)
    {
        if (_uriString->text().isEmpty())
            return;

        on_generateButton_clicked();
    }
    */
}

