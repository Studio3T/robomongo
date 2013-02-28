#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"

#include <QtGui>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

CreateDatabaseDialog::CreateDatabaseDialog(const QString &serverName, QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Create Database");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
    setMinimumWidth(300);

    Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);

    QFrame *hline = new QFrame();
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);

    _databaseName = new QLineEdit();
    QLabel *label = new QLabel("Database Name:");

    QPushButton *cancelButton = new QPushButton("&Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

    QPushButton *createButton = new QPushButton("&Create");
    connect(createButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addStretch(1);
    hlayout->addWidget(createButton, 0, Qt::AlignRight);
    hlayout->addWidget(cancelButton, 0, Qt::AlignRight);

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
    vlayout->addWidget(hline);

    QVBoxLayout *namelayout = new QVBoxLayout();
    namelayout->setContentsMargins(0, 7, 0, 7);
    namelayout->addWidget(label);
    namelayout->addWidget(_databaseName);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(vlayout);
    layout->addLayout(namelayout);
    layout->addLayout(hlayout);
    setLayout(layout);
}

QString CreateDatabaseDialog::databaseName() const
{
    return _databaseName->text();
}

void CreateDatabaseDialog::accept()
{
    if (_databaseName->text().isEmpty())
        return;

    QDialog::accept();
}
