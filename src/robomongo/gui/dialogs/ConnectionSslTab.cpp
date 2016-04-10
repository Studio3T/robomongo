#include "robomongo/gui/dialogs/ConnectionSslTab.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QRegExpValidator>
#include <QFileDialog>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    ConnectionSslTab::ConnectionSslTab(ConnectionSettings *settings) :
        _settings(settings)
    {
        _sslPemFilePath = new QLineEdit(QtUtils::toQString(_settings->sslInfo()._sslPEMKeyFile));

/*
// Commented because of this:
// https://github.com/paralect/robomongo/issues/391

#ifdef Q_OS_WIN
        QRegExp pathx("([a-zA-Z]:)?([\\\\/][a-zA-Z0-9_.-]+)+[\\\\/]?");
#else
        QRegExp pathx("^\\/?([\\d\\w\\.]+)(/([\\d\\w\\.]+))*\\/?$");
#endif // Q_OS_WIN

        _sslPemFilePath->setValidator(new QRegExpValidator(pathx, this));
*/
        _sslSupport = new QCheckBox("Use SSL protocol");
        _sslSupport->setStyleSheet("margin-bottom: 7px");
        _sslSupport->setChecked(_settings->sslInfo()._sslSupport);

        _selectPemFileButton = new QPushButton("...");
        _selectPemFileButton->setFixedSize(20, 20);
        VERIFY(connect(_selectPemFileButton, SIGNAL(clicked()), this, SLOT(setSslPEMKeyFile())));

        _sslPemLabel = new QLabel("SSL Certificate:");

        _sslPemDescriptionLabel = new QLabel(
            "<nobr>SSL Certificate and SSL Private Key combined into one file (*.pem)");
        _sslPemDescriptionLabel->setWordWrap(true);
        _sslPemDescriptionLabel->setAlignment(Qt::AlignTop);
        _sslPemDescriptionLabel->setContentsMargins(0, -2, 0, 0);
        _sslPemDescriptionLabel->setMinimumSize(_sslPemDescriptionLabel->sizeHint());

        QLabel *sslNoteLabel = new QLabel(
            "<b>Note:</b> Support for Certificate Authority (CA) file and "
            "encrypted SSL Private Keys are planned for future releases.");
        sslNoteLabel->setWordWrap(true);
        sslNoteLabel->setAlignment(Qt::AlignTop);
        sslNoteLabel->setContentsMargins(0, 20, 0, 0);

        QGridLayout *_authLayout = new QGridLayout;
        _authLayout->setAlignment(Qt::AlignTop);
        _authLayout->addWidget(_sslSupport,              0, 0, 1, 3);
        _authLayout->addWidget(_sslPemLabel,             1, 0);
        _authLayout->addWidget(_sslPemFilePath,          1, 1, 1, 1);
        _authLayout->addWidget(_selectPemFileButton,     1, 2, 1, 1);
        _authLayout->addWidget(_sslPemDescriptionLabel,  2, 1, 1, 2);
        _authLayout->addWidget(sslNoteLabel,             3, 0, 1, 4);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addLayout(_authLayout);
        setLayout(mainLayout);

        sslSupportStateChange(_sslSupport->checkState());
        VERIFY(connect(_sslSupport, SIGNAL(stateChanged(int)), this, SLOT(sslSupportStateChange(int))));
    }

    void ConnectionSslTab::setSslPEMKeyFile()
    {
        QString filepath = QFileDialog::getOpenFileName(this, "Select SSL Key File (PEM)", "", QObject::tr("PEM files (*.pem)"));
        if (filepath.isNull())
            return;

        _sslPemFilePath->setText(filepath);
    }

    void ConnectionSslTab::sslSupportStateChange(int value)
    {
        _sslPemFilePath->setEnabled(value);
        _selectPemFileButton->setEnabled(value);
        _sslPemDescriptionLabel->setEnabled(value);
        _sslPemLabel->setEnabled(value);

        if (!value) {
            _sslPemFilePath->setText("");
        }
    }

    bool ConnectionSslTab::isSslSupported() const
    {
        bool result = true;
#ifdef MONGO_SSL
        result = _sslSupport->isChecked();
#endif
      return result;
    }

    void ConnectionSslTab::accept()
    {
#ifdef MONGO_SSL
        SSLInfo inf(_sslSupport->isChecked(), QtUtils::toStdString(_sslPemFilePath->text()));
        _settings->setSslInfo(inf);
#endif // MONGO_SSL
    }
}
