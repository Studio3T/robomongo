#include "robomongo/gui/dialogs/EditIndexDialog.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"

namespace Robomongo
{
    EditItemIndexDialog::EditItemIndexDialog(QWidget *parent,const QString &text,const QString &collectionName,const QString &databaseName):BaseClass(parent)
    {        
        Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), collectionName);
        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), databaseName);

        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setContentsMargins(0, 0, 0, 0);
        hlayout->setSpacing(0);
        hlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
        hlayout->addWidget(collectionIndicator, 0, Qt::AlignLeft);
        hlayout->addStretch(1);

        QLabel *label = new QLabel(tr("Name:"));
        _indexName = new QLineEdit(this);
        _indexName->setText(text);
        QHBoxLayout *nameLayout = new QHBoxLayout;
        nameLayout->addWidget(label);
        nameLayout->addWidget(_indexName);

        QDialogButtonBox *buttonBox = new QDialogButtonBox (this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons (QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(5, 5, 5, 5);
        vlayout->addLayout(hlayout);
        vlayout->addLayout(nameLayout);
        vlayout->addWidget(buttonBox);

        setLayout(vlayout);
        setFixedSize(WidthWidget,HeightWidget);
    }

    QString EditItemIndexDialog::text()const
    {
        return _indexName->text();
    }

    void EditItemIndexDialog::accept()
    {
        if(!text().isEmpty()){
            return BaseClass::accept();
        }
        else{
            QMessageBox::warning(this, "Invalid index name", "Please check index name.\n");
            _indexName->setFocus();
        }
    }
}
