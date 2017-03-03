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
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>

#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{

    EulaDialog::EulaDialog(QWidget *parent)
        : QWizard(parent)
    {
        setWindowTitle("Thank you!");

        QSettings settings("3T", "RoboM");
        if (settings.contains("EulaDialog/size"))
            restoreWindowSettings();

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
        radioButtonsLay->addWidget(agreeButton/*, Qt::AlignCenter*/);
        radioButtonsLay->addWidget(notAgreeButton/*, Qt::AlignCenter*/);

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

        auto bodyLabel = new QLabel("Share your email address with us and we'll keep you"
            "up-to-date with updates from us and new features as they come out.");
        bodyLabel->setWordWrap(true);

        auto mainLayout2 = new QGridLayout();
        mainLayout2->addWidget(new QLabel,          0, 0, 1, 2);
        mainLayout2->addWidget(new QLabel("<h3>Thank you for choosing RoboM!</h3>"), 1, 0, 1, 2);
        mainLayout2->addWidget(bodyLabel,           2, 0 , 1, 2);
        mainLayout2->addWidget(new QLabel,          3, 0, 1, 2);
        mainLayout2->addWidget(nameLabel,           4, 0);
        mainLayout2->addWidget(_nameEdit,            4, 1);
        mainLayout2->addWidget(lastNameLabel,       5, 0);
        mainLayout2->addWidget(_lastNameEdit,        5, 1);
        mainLayout2->addWidget(emailLabel,          6, 0);
        mainLayout2->addWidget(_emailEdit,           6, 1);
        mainLayout2->addWidget(new QLabel,          7, 0, 1, 2);
        mainLayout2->addWidget(buttomLabel,         8, 0, 1, 2);

        secondPage->setLayout(mainLayout2);

#ifndef _WIN32
        addPage(firstPage);     // Disable license page on Windows, it is already shown in installer
#endif
        addPage(secondPage);

        //// Buttons
        setButtonText(QWizard::CustomButton1, tr("Back"));
        setButtonText(QWizard::CustomButton2, tr("Next"));
        setButtonText(QWizard::CustomButton3, tr("Finish"));

        VERIFY(connect(button(QWizard::CustomButton1), SIGNAL(clicked()), this, SLOT(on_back_clicked())));
        VERIFY(connect(button(QWizard::CustomButton2), SIGNAL(clicked()), this, SLOT(on_next_clicked())));
        VERIFY(connect(button(QWizard::CustomButton3), SIGNAL(clicked()), this, SLOT(on_finish_clicked())));

        setButtonLayout(QList<WizardButton>{ QWizard::Stretch, QWizard::CustomButton1, QWizard::CustomButton2,
            QWizard::CancelButton, QWizard::CustomButton3});

#ifdef _WIN32
        button(QWizard::CustomButton1)->setHidden(true);
        button(QWizard::CustomButton2)->setHidden(true);
        button(QWizard::CustomButton3)->setEnabled(true);
#else
        button(QWizard::CustomButton1)->setDisabled(true);
        button(QWizard::CustomButton2)->setDisabled(true);
        button(QWizard::CustomButton3)->setDisabled(true);
#endif

        setWizardStyle(QWizard::ModernStyle);

        setMinimumSize(sizeHint().width()*1.3, sizeHint().height()*1.3);
    }

    void EulaDialog::accept()
    {
        saveWindowSettings();

        // Build post data and send
        QJsonObject jsonStr{
            { "email", _emailEdit->text() },
            { "firstName", _nameEdit->text() },
            { "lastName", _lastNameEdit->text() }
        };

        QJsonDocument jsonDoc(jsonStr);
        QUrlQuery postData(jsonDoc.toJson());
        postData = QUrlQuery("rd=" + postData.toString(QUrl::FullyEncoded).toUtf8());

        QNetworkRequest request(QUrl("https://rm-form.3t.io/"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        auto networkManager = new QNetworkAccessManager(this);
        VERIFY(connect(networkManager, SIGNAL(finished(QNetworkReply*)), SLOT(on_postAnswer(QNetworkReply*))));
        _reply = networkManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
        connect(_reply, SIGNAL(readyRead()), this, SLOT(postReplyReadyRead()));
        connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(postReplyError(QNetworkReply::NetworkError)));

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
        button(QWizard::CustomButton2)->setEnabled(true);
    }

    void EulaDialog::on_notAgreeButton_clicked()
    {
        button(QWizard::CustomButton2)->setEnabled(false);
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

    void EulaDialog::on_postAnswer(QNetworkReply* reply)
    {
        //todo
        //auto xx = reply->readAll().toStdString();
    }

    void EulaDialog::postReplyReadyRead()
    {
        // todo
        //auto xx = _reply->readAll().toStdString();
    }

    void EulaDialog::postReplyError(QNetworkReply::NetworkError error)
    {
        // todo
        //auto xx = 55;
        //++xx;
    }

    void EulaDialog::saveWindowSettings() const
    {
        QSettings settings("3T", "RoboM");
        settings.setValue("EulaDialog/size", size());
    }

    void EulaDialog::restoreWindowSettings()
    {
        QSettings settings("3T", "RoboM");
        resize(settings.value("EulaDialog/size").toSize());
    }

}
