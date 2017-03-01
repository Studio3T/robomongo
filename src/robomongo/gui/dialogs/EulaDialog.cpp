#include "robomongo/gui/dialogs/EulaDialog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QSettings>
#include <QLabel>
#include <QCheckBox>
#include <QTextBrowser>
#include <QLineEdit>
#include <QRadioButton>

#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{

    EulaDialog::EulaDialog(QWidget *parent) 
        : QWizard(parent)
    {
        setWindowTitle("Studio 3T Robo: Let's get started");

        QSettings settings("Paralect", "Robomongo");
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

        auto firstCheckBox = new QCheckBox("Send me helpful tips and tricks during evaluation");
        auto secondCheckBox = new QCheckBox("Tell me about new product features as they come out");

        auto nameLabel = new QLabel("<b>First Name</b>");
        auto nameEdit = new QLineEdit;
        auto lastNameLabel = new QLabel("<b>Last Name</b>");
        auto lastNameEdit = new QLineEdit;

        auto nameLay = new QVBoxLayout;
        nameLay->addWidget(nameLabel);
        nameLay->addWidget(nameEdit);

        auto lastNameLay = new QVBoxLayout;
        lastNameLay->addWidget(lastNameLabel);
        lastNameLay->addWidget(lastNameEdit);

        auto namesLay = new QHBoxLayout;
        namesLay->addLayout(nameLay);
        namesLay->addLayout(lastNameLay);

        auto emailLabel = new QLabel("<b>Email</b>");
        auto emailEdit = new QLineEdit;

        auto buttomLabel = new QLabel("By submitting this form I agree to 3T Software Labs " 
                                      "<a href='http://studio3t.com/privacy-policy'>Privacy Policy</a>.");
        buttomLabel->setOpenExternalLinks(true);

        auto mainLayout2 = new QVBoxLayout();
        mainLayout2->addWidget(new QLabel("<h3>New to Studio 3T Robo?</h3>"));
        mainLayout2->addWidget(new QLabel);
        mainLayout2->addWidget(firstCheckBox);
        mainLayout2->addWidget(secondCheckBox);
        mainLayout2->addWidget(new QLabel);
        mainLayout2->addLayout(namesLay);
        mainLayout2->addWidget(emailLabel);
        mainLayout2->addWidget(emailEdit);
        mainLayout2->addWidget(new QLabel);
        mainLayout2->addWidget(buttomLabel);
        mainLayout2->addStretch();

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

    void EulaDialog::saveWindowSettings() const
    {
        QSettings settings("Paralect", "Robomongo");
        settings.setValue("EulaDialog/size", size());
    }

    void EulaDialog::restoreWindowSettings()
    {
        QSettings settings("Paralect", "Robomongo");
        resize(settings.value("EulaDialog/size").toSize());
    }

}
