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
#include <QTextEdit>

#include "robomongo/core/utils/QtUtils.h"


namespace Robomongo
{

    EulaDialog::EulaDialog(QWidget *parent) 
        : QDialog(parent)
    {
        setWindowTitle("Robomongo Setup");

        QSettings settings("Paralect", "Robomongo");
        if (settings.contains("EulaDialog/size"))
            restoreWindowSettings();
        else
            resize(400, 600);

        setWindowFlags(Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

        _checkBox = new QCheckBox("I accept the terms in the License Agreement.");
        VERIFY(connect(_checkBox, SIGNAL(stateChanged(int)),
                       this, SLOT(enableOkButton(int))));

        _buttonBox = new QDialogButtonBox(this);
        _buttonBox->setOrientation(Qt::Horizontal);
        _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        _buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        VERIFY(connect(_buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        auto textEdit = new QTextEdit;
        textEdit->setFont(QFont("Arial", 14));
        QFile file(":eula.html");
        if (file.open(QFile::ReadOnly | QFile::Text))
            textEdit->setText(file.readAll());

        auto hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        auto mainLayout = new QVBoxLayout();
        mainLayout->addWidget(new QLabel("<h3>End-User License Agreement</h3>"));
        mainLayout->addWidget(new QLabel("Please read the following license agreement carefully."));
        mainLayout->addWidget(new QLabel(""));
        mainLayout->addWidget(textEdit);
        mainLayout->addWidget(new QLabel(""));
        mainLayout->addWidget(_checkBox, Qt::AlignLeft);
        mainLayout->addWidget(new QLabel(""));
        mainLayout->addWidget(hline);
        mainLayout->addWidget(_buttonBox, Qt::AlignLeft);
        setLayout(mainLayout);

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

    void EulaDialog::enableOkButton(int state)
    {
        _buttonBox->button(QDialogButtonBox::Ok)->setEnabled(state);
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
