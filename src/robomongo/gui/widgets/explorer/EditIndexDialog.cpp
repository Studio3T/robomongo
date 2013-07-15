#include "robomongo/gui/widgets/explorer/EditIndexDialog.h"

#include <QTabWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QSpacerItem>
#include <QTextEdit>

namespace Robomongo
{
    EditIndexDialog::EditIndexDialog(QWidget *parent, ExplorerCollectionTreeItem * const item):BaseClass(parent),_item(item)
    {
        QLabel *label = new QLabel(tr("Name:"));
        _nameLineEdit = new QLineEdit(this);
        QHBoxLayout *nameLayout = new QHBoxLayout;
        nameLayout->addWidget(label);
        nameLayout->addWidget(_nameLineEdit);

        _mainTab = new QTabWidget(this);
        _jsonText = new QTextEdit(this);
        _mainTab->addTab(_jsonText,tr("Json"));
        _mainTab->addTab(new QWidget(this),tr("Visual"));
        _mainTab->setTabsClosable(false);
       
        
        QHBoxLayout *checkBoxLayout = new QHBoxLayout;
        QSpacerItem *sp = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        checkBoxLayout->addItem(sp);
        checkBoxLayout->addWidget(new QCheckBox(tr("Unique"), this));
        checkBoxLayout->addWidget(new QCheckBox(tr("Drop duplicates"), this));
        checkBoxLayout->addWidget(new QCheckBox(tr("Create index in background"), this));

        QDialogButtonBox *buttonBox = new QDialogButtonBox (this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons (QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(0, 0, 0, 0);
        vlayout->addLayout(nameLayout);
        vlayout->addWidget(_mainTab);
        vlayout->addLayout(checkBoxLayout);
        vlayout->addWidget(buttonBox);


        setLayout(vlayout);
        setFixedSize(WidthWidget,HeightWidget);
    }
}