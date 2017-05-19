#include "robomongo/gui/dialogs/EulaDialog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QSettings>
#include <QLabel>
#include <QTextBrowser>
#include <QLineEdit>
#include <QRadioButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDesktopWidget>

#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{

    EulaDialog::EulaDialog(QWidget *parent)
        : QWizard(parent)
    {
        setWindowTitle("EULA");

        //// First page
        auto firstPage = new QWizardPage;

        auto agreeButton = new QRadioButton("I agree");
        VERIFY(connect(agreeButton, SIGNAL(clicked()),
            this, SLOT(on_agreeButton_clicked())));

        auto notAgreeButton = new QRadioButton("I don't agree");
        notAgreeButton->setChecked(true);
        VERIFY(connect(notAgreeButton, SIGNAL(clicked()),
            this, SLOT(on_notAgreeButton_clicked())));

        auto radioButtonsLay = new QHBoxLayout;
        radioButtonsLay->setAlignment(Qt::AlignHCenter);
        radioButtonsLay->setSpacing(30);
        radioButtonsLay->addWidget(agreeButton);
        radioButtonsLay->addWidget(notAgreeButton);

        auto textBrowser = new QTextBrowser;
        textBrowser->setOpenExternalLinks(true);
        textBrowser->setOpenLinks(true);
        QFile file(":gnu_gpl3_license.html");
        if (file.open(QFile::ReadOnly | QFile::Text))
            textBrowser->setText(file.readAll());

        auto hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        auto mainLayout1 = new QVBoxLayout();
        mainLayout1->addWidget(new QLabel("<h3>End-User License Agreement</h3>"));
        mainLayout1->addWidget(new QLabel(""));
        mainLayout1->addWidget(textBrowser);
        mainLayout1->addWidget(new QLabel(""));
        mainLayout1->addLayout(radioButtonsLay, Qt::AlignCenter);
        mainLayout1->addWidget(new QLabel(""));
        mainLayout1->addWidget(hline);

        firstPage->setLayout(mainLayout1);

        //// Second page
        auto secondPage = new QWizardPage;

        auto nameLabel = new QLabel("<b>First Name:</b>");
        _nameEdit = new QLineEdit;
        auto lastNameLabel = new QLabel("<b>Last Name:</b>");
        _lastNameEdit = new QLineEdit;

        auto emailLabel = new QLabel("<b>Email:</b>");
        _emailEdit = new QLineEdit;

        auto buttomLabel = new QLabel("By submitting this form I agree to 3T Software Labs "
            "<a href='https://studio3t.com/privacy-policy'>Privacy Policy</a>.");
        buttomLabel->setOpenExternalLinks(true);

        auto bodyLabel = new QLabel("\nShare your email address with us and we'll keep you "
            "up-to-date with updates from us and new features as they come out.");
        bodyLabel->setWordWrap(true);

        auto mainLayout2 = new QGridLayout();
        mainLayout2->addWidget(new QLabel,          0, 0, 1, 2);
        mainLayout2->addWidget(new QLabel("<h3>Thank you for choosing Robo 3T!</h3>"), 1, 0, 1, 2);
        mainLayout2->addWidget(bodyLabel,           2, 0 , 1, 2);
        mainLayout2->addWidget(new QLabel,          3, 0, 1, 2);
        mainLayout2->addWidget(nameLabel,           4, 0);
        mainLayout2->addWidget(_nameEdit,           4, 1);
        mainLayout2->addWidget(lastNameLabel,       5, 0);
        mainLayout2->addWidget(_lastNameEdit,       5, 1);
        mainLayout2->addWidget(emailLabel,          6, 0);
        mainLayout2->addWidget(_emailEdit,          6, 1);
        mainLayout2->addWidget(new QLabel,          7, 0, 1, 2);
        mainLayout2->addWidget(buttomLabel,         8, 0, 1, 2);

        secondPage->setLayout(mainLayout2);

        addPage(firstPage);
#if defined(_WIN32) || defined(__APPLE__)
        addPage(secondPage);
#endif

        //// Buttons
        setButtonText(QWizard::CustomButton1, tr("Back"));
        setButtonText(QWizard::CustomButton2, tr("Next"));
        setButtonText(QWizard::CustomButton3, tr("Finish"));

        VERIFY(connect(button(QWizard::CustomButton1), SIGNAL(clicked()), this, SLOT(on_back_clicked())));
        VERIFY(connect(button(QWizard::CustomButton2), SIGNAL(clicked()), this, SLOT(on_next_clicked())));
        VERIFY(connect(button(QWizard::CustomButton3), SIGNAL(clicked()), this, SLOT(on_finish_clicked())));

        setButtonLayout(QList<WizardButton>{ QWizard::Stretch, QWizard::CustomButton1, QWizard::CustomButton2,
            QWizard::CancelButton, QWizard::CustomButton3});

#if defined(_WIN32) || defined(__APPLE__)
        button(QWizard::CustomButton1)->setDisabled(true);
        button(QWizard::CustomButton2)->setDisabled(true);
        button(QWizard::CustomButton3)->setDisabled(true);
#else   // linux
        button(QWizard::CustomButton1)->setHidden(true);
        button(QWizard::CustomButton2)->setHidden(true);
        button(QWizard::CustomButton3)->setDisabled(true);
#endif

        setWizardStyle(QWizard::ModernStyle);

        QSettings const settings("3T", "Robomongo");
        if (settings.contains("EulaDialog/size")) {
            restoreWindowSettings();
        }
        else {
            auto const desktop = QApplication::desktop();
            auto const& mainScreenSize = desktop->availableGeometry(desktop->primaryScreen()).size();
            resize(mainScreenSize.width()*0.5, mainScreenSize.height()*0.6);
        }
    }

    void EulaDialog::accept()
    {
        saveWindowSettings();

        // Build post data and send
        postUserData();

        QDialog::accept();
    }

    void EulaDialog::reject()
    {
        saveWindowSettings();
        QDialog::reject();
    }

    void EulaDialog::closeEvent(QCloseEvent *event)
    {
        saveWindowSettings();
        QWidget::closeEvent(event);
    }

    void EulaDialog::on_agreeButton_clicked()
    {
#if defined(_WIN32) || defined(__APPLE__)
        button(QWizard::CustomButton2)->setEnabled(true);
#else
        button(QWizard::CustomButton3)->setEnabled(true);
#endif
    }

    void EulaDialog::on_notAgreeButton_clicked()
    {
#if defined(_WIN32) || defined(__APPLE__)
        button(QWizard::CustomButton2)->setEnabled(false);
#else
        button(QWizard::CustomButton3)->setEnabled(false);
#endif
    }

    void EulaDialog::on_next_clicked()
    {
        next();
        button(QWizard::CustomButton1)->setEnabled(true);
        button(QWizard::CustomButton2)->setEnabled(false);
        button(QWizard::CustomButton3)->setEnabled(true);
    }

    void EulaDialog::on_back_clicked()
    {
        back();
        button(QWizard::CustomButton1)->setEnabled(false);
        button(QWizard::CustomButton2)->setEnabled(true);
        button(QWizard::CustomButton3)->setEnabled(false);
    }

    void EulaDialog::on_finish_clicked()
    {
        accept();
    }

    void EulaDialog::postUserData() const
    {
        if (_emailEdit->text().isEmpty())
            return;

        // Build post data and send
        QJsonObject jsonStr {
            { "email", _emailEdit->text() },
            { "firstName", _nameEdit->text() },
            { "lastName", _lastNameEdit->text() }
        };

        QJsonDocument jsonDoc(jsonStr);
        QUrlQuery postData(jsonDoc.toJson());
        postData = QUrlQuery("rd=" + postData.toString(QUrl::FullyEncoded).toUtf8());

        QNetworkRequest request(QUrl("https://rm-form.3t.io/"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        auto networkManager = new QNetworkAccessManager;
        _reply = networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    }

    void EulaDialog::saveWindowSettings() const
    {
        QSettings settings("3T", "Robomongo");
        settings.setValue("EulaDialog/size", size());
    }

    void EulaDialog::restoreWindowSettings()
    {
        QSettings settings("3T", "Robomongo");
        resize(settings.value("EulaDialog/size").toSize());
    }

}
