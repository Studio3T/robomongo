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
#include <QMessageBox>
#include <qjson/parser.h>

namespace
{
    bool isValidJson(const QString &text)
    {
        bool result= false;
        if(!text.isEmpty()){
            QJson::Parser parser;
            bool ok=false;
            QVariant var = parser.parse(text.toUtf8(), &ok);
            if(ok){
               QMap<QString,QVariant> m= var.toMap();
               result = m.size()!=0;
            }
        }
        return result;
    }
}

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
        //_mainTab->addTab(new QWidget(this),tr("Visual"));
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
    void EditIndexDialog::accept()
    {
        if(_nameLineEdit->text().isEmpty()){
            QMessageBox::warning(this, "Empty name", "Please input name for index \n");
            _nameLineEdit->setFocus();
        }
        if(isValidJson(_jsonText->toPlainText())){
            return BaseClass::accept();
        }
        else
        {
            QMessageBox::warning(this, "Invalid json", "Please check json text.\n");
            _jsonText->setFocus();
        }
    }
    QString EditIndexDialog::getInputText()const
    {
        return QString(" %1 ,{ name: %2 }").arg(_jsonText->toPlainText()).arg(_nameLineEdit->text());
    }
}
