#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"

#include <QtGui>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

CreateDatabaseDialog::CreateDatabaseDialog(const QString &serverName, const QString &database,
                                           const QString &collection, QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Create Database");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
    setMinimumWidth(300);

    Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);
    Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);
    Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), collection);

    QFrame *hline = new QFrame();
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);

    _inputEdit = new QLineEdit();
    _inputLabel= new QLabel("Database Name:");

    QPushButton *cancelButton = new QPushButton("&Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

    _okButton = new QPushButton("&Create");
    connect(_okButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addStretch(1);
    hlayout->addWidget(_okButton, 0, Qt::AlignRight);
    hlayout->addWidget(cancelButton, 0, Qt::AlignRight);

    QHBoxLayout *vlayout = new QHBoxLayout();
    if (!serverName.isEmpty())
        vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
    if (!database.isEmpty())
        vlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
    if (!collection.isEmpty())
        vlayout->addWidget(collectionIndicator, 0, Qt::AlignLeft);

    QVBoxLayout *namelayout = new QVBoxLayout();
    namelayout->setContentsMargins(0, 7, 0, 7);
    namelayout->addWidget(_inputLabel);
    namelayout->addWidget(_inputEdit);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(vlayout);
    layout->addWidget(hline);
    layout->addLayout(namelayout);
    layout->addLayout(hlayout);
    setLayout(layout);
}

QString CreateDatabaseDialog::databaseName() const
{
    return _inputEdit->text();
}

void CreateDatabaseDialog::setOkButtonText(const QString &text)
{
    _okButton->setText(text);
}

void CreateDatabaseDialog::setInputLabelText(const QString &text)
{
    _inputLabel->setText(text);
}

void CreateDatabaseDialog::setInputText(const QString &text)
{
    _inputEdit->setText(text);
    _inputEdit->selectAll();
}

void CreateDatabaseDialog::accept()
{
    if (_inputEdit->text().isEmpty())
        return;

    QDialog::accept();
}
