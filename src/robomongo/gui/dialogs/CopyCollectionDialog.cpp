#include "robomongo/gui/dialogs/CopyCollectionDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    CopyCollection::CopyCollection(const QString &serverName, const QString &database,
                                               const QString &collection, QWidget *parent) :
        QDialog(parent)
    {
        setWindowTitle("Create Database");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        setMinimumWidth(300);

        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);

        QFrame *hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        _buttonBox = new QDialogButtonBox(this);
        _buttonBox->setOrientation(Qt::Horizontal);
        _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        _buttonBox->button(QDialogButtonBox::Save)->setText("Copy");
        VERIFY(connect(_buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->addStretch(1);
        hlayout->addWidget(_buttonBox);

        QHBoxLayout *vlayout = new QHBoxLayout();
        if (!serverName.isEmpty())
            vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        if (!database.isEmpty())
            vlayout->addWidget(new Indicator(GuiRegistry::instance().databaseIcon(), database), 0, Qt::AlignLeft);
        if (!collection.isEmpty())
            vlayout->addWidget(new Indicator(GuiRegistry::instance().collectionIcon(), collection), 0, Qt::AlignLeft);

        QVBoxLayout *serverlayout = new QVBoxLayout();
        _server = new QComboBox();
        QLabel *serverLabel = new QLabel("Select server");
        serverlayout->addWidget(serverLabel);
        serverlayout->addWidget(_server);

        QVBoxLayout *databaselayout = new QVBoxLayout();
        _database = new QComboBox();
        QLabel *databaseLabel = new QLabel("Select database");
        databaselayout->addWidget(databaseLabel);
        databaselayout->addWidget(_database);        

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addLayout(vlayout);
        layout->addWidget(hline);
        layout->addLayout(serverlayout);
        layout->addLayout(databaselayout);
        layout->addLayout(hlayout);
        setLayout(layout);
    }

    void CopyCollection::accept()
    {
        QDialog::accept();
    }
}
