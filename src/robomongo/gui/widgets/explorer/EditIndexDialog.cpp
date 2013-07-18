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
#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/core/domain/MongoCollection.h"

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
        Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), _item->collection()->name());
        ExplorerDatabaseTreeItem *const database = dynamic_cast<ExplorerDatabaseTreeItem *const>(_item->databaseItem());
        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database->database()->name());

        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setContentsMargins(0, 0, 0, 0);
        hlayout->setSpacing(0);
        hlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
        hlayout->addWidget(collectionIndicator, 0, Qt::AlignLeft);
        hlayout->addStretch(1);

        QLabel *label = new QLabel(tr("Name:"));
        _nameLineEdit = new QLineEdit(this);
        QHBoxLayout *nameLayout = new QHBoxLayout;
        nameLayout->addWidget(label);
        nameLayout->addWidget(_nameLineEdit);

        QTabWidget *mainTab = new QTabWidget(this);

        QFont textFont = font();
        #if defined(Q_OS_MAC)
                textFont.setPointSize(12);
                textFont.setFamily("Monaco");
        #elif defined(Q_OS_UNIX)
                textFont.setFamily("Monospace");
                textFont.setFixedPitch(true);
                //textFont.setWeight(QFont::Bold);
                //    textFont.setPointSize(12);
        #elif defined(Q_OS_WIN)
                textFont.setPointSize(10);
                textFont.setFamily("Courier");
        #endif

        QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
        javaScriptLexer->setFont(textFont);

        _jsonText = new FindFrame(this);
        _jsonText->sciScintilla()->setLexer(javaScriptLexer);
        _jsonText->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);        
        _jsonText->sciScintilla()->setTabWidth(4);        
        _jsonText->sciScintilla()->setBraceMatching(QsciScintilla::StrictBraceMatch);
        _jsonText->sciScintilla()->setFont(textFont);
        _jsonText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);
        _jsonText->sciScintilla()->setStyleSheet("QFrame {background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 0px; margin: 0px; padding: 0px;}");

        mainTab->addTab(_jsonText,tr("Json"));
        //_mainTab->addTab(new QWidget(this),tr("Visual"));
        mainTab->setTabsClosable(false);
       
        QHBoxLayout *checkBoxLayout = new QHBoxLayout;
        QSpacerItem *sp = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        checkBoxLayout->addItem(sp);
        _uniqueCheckBox = new QCheckBox(tr("Unique"),this);
        _dropDuplicates = new QCheckBox(tr("Drop duplicates"),this);
        _backGroundCheckBox = new QCheckBox(tr("Create index in background"),this);
        checkBoxLayout->addWidget(_uniqueCheckBox);
        checkBoxLayout->addWidget(_dropDuplicates);
        checkBoxLayout->addWidget(_backGroundCheckBox);

        QDialogButtonBox *buttonBox = new QDialogButtonBox (this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons (QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(5, 5, 5, 5);
        vlayout->addLayout(hlayout);
        vlayout->addLayout(nameLayout);
        vlayout->addWidget(mainTab);
        vlayout->addLayout(checkBoxLayout);
        vlayout->addWidget(buttonBox);

        setLayout(vlayout);
        setFixedSize(WidthWidget,HeightWidget);
    }

    QString EditIndexDialog::indexName() const
    {
        return _nameLineEdit->text();
    }

    bool EditIndexDialog::isUnique() const
    {
        return _uniqueCheckBox->checkState() == Qt::Checked;
    }

    bool EditIndexDialog::isBackGround() const
    {
        return _backGroundCheckBox->checkState() == Qt::Checked;
    }

    bool EditIndexDialog::isDropDuplicates() const
    {
        return _dropDuplicates->checkState() == Qt::Checked;
    }

    void EditIndexDialog::accept()
    {
        if (_nameLineEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Empty name", "Please input name for index \n");
            _nameLineEdit->setFocus();
        }
        if (isValidJson(_jsonText->sciScintilla()->text())) {
            return BaseClass::accept();
        }
        else {
            QMessageBox::warning(this, "Invalid json", "Please check json text.\n");
            _jsonText->setFocus();
        }
    }

    QString EditIndexDialog::getInputText() const
    {
        return QString(" %1 ,{ name: %2 }").arg(_jsonText->sciScintilla()->text()).arg(_nameLineEdit->text());
    }
}
