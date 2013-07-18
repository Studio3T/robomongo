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
#include <QMessageBox>
#include <Qsci/qsciscintilla.h>

#include "robomongo/shell/db/json.h"
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
            try {
                mongo::Robomongo::fromjson(text.toUtf8());
                result=true;
            }
            catch (const mongo::ParseMsgAssertionException &ex) {

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

        QTabWidget *mainTab = new QTabWidget(this);

        mainTab->addTab(createBasicTab(),tr("Basic"));
        mainTab->addTab(createAdvancedTab(),tr("Advanced"));
        mainTab->addTab(createTextSearchTab(),tr("Text Search"));
        mainTab->setTabsClosable(false);

        QDialogButtonBox *buttonBox = new QDialogButtonBox (this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons (QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(0, 0, 0, 0);
        vlayout->addLayout(hlayout);
        vlayout->addWidget(mainTab);
        vlayout->addWidget(buttonBox);

        setLayout(vlayout);
        setFixedSize(WidthWidget,HeightWidget);
    }

    QWidget* EditIndexDialog::createBasicTab()
    {
        QWidget *basicTab = new QWidget(this);

        QLabel *label = new QLabel(tr("Name:"),basicTab);
        _nameLineEdit = new QLineEdit(basicTab);
        QHBoxLayout *nameLayout = new QHBoxLayout;
        nameLayout->addWidget(label);
        nameLayout->addWidget(_nameLineEdit);

        const QFont &textFont = GuiRegistry::instance().font();
        QsciLexerJavaScript *javaScriptLexer = new JSLexer(basicTab);
        javaScriptLexer->setFont(textFont);
        _jsonText = new FindFrame(basicTab);
        _jsonText->sciScintilla()->setLexer(javaScriptLexer);
        _jsonText->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);        
        _jsonText->sciScintilla()->setTabWidth(4);        
        _jsonText->sciScintilla()->setBraceMatching(QsciScintilla::StrictBraceMatch);
        _jsonText->sciScintilla()->setFont(textFont);
        _jsonText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);
        _jsonText->sciScintilla()->setStyleSheet("QFrame {background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 0px; margin: 0px; padding: 0px;}");

        QHBoxLayout *checkBoxLayout = new QHBoxLayout;
        QSpacerItem *sp = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        _uniqueCheckBox = new QCheckBox(tr("Unique"),basicTab);
        checkBoxLayout->addWidget(_uniqueCheckBox);
        checkBoxLayout->addItem(sp);        

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(5, 5, 5, 5);
        vlayout->addLayout(nameLayout);
        vlayout->addWidget(_jsonText);
        vlayout->addLayout(checkBoxLayout);
        basicTab->setLayout(vlayout);

        return basicTab;
    }

    QWidget* EditIndexDialog::createAdvancedTab()
    {
        QWidget *advanced = new QWidget(this);

        _sparceCheckBox = new QCheckBox(tr("Sparse"),advanced);
        _dropDuplicates = new QCheckBox(tr("Drop duplicates"),advanced);
        _backGroundCheckBox = new QCheckBox(tr("Create index in background"),advanced);
        
        QHBoxLayout *expireLayout = new QHBoxLayout;
        QLabel *exLabel = new QLabel(tr("Expire after"));
        _expireAfterLineEdit = new QLineEdit(advanced);
        QLabel *secLabel = new QLabel(tr("seconds"),advanced);  
        expireLayout->addWidget(exLabel);
        expireLayout->addWidget(_expireAfterLineEdit);
        expireLayout->addWidget(secLabel);

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(5, 5, 5, 5);
        vlayout->addWidget(_sparceCheckBox);
        vlayout->addWidget(_dropDuplicates);
        vlayout->addWidget(_backGroundCheckBox);
        vlayout->addLayout(expireLayout);
        advanced->setLayout(vlayout);

        return advanced;
    }

    QWidget* EditIndexDialog::createTextSearchTab()
    {
        QWidget *textSearch = new QWidget(this);

        QHBoxLayout *defLangLayout = new QHBoxLayout;
        QLabel *defaultLanguage = new QLabel(tr("Default language"),textSearch);
        _defaultLanguageLineEdit = new QLineEdit(textSearch);
        defLangLayout->addWidget(defaultLanguage);
        defLangLayout->addWidget(_defaultLanguageLineEdit);

        QHBoxLayout *languageOverrideLayout = new QHBoxLayout;
        QLabel *languageOverrideLabel = new QLabel(tr("Language override"),textSearch);
        _languageOverrideLineEdit = new QLineEdit(textSearch);
        languageOverrideLayout->addWidget(languageOverrideLabel);
        languageOverrideLayout->addWidget(_languageOverrideLineEdit);

        QLabel *textWeights = new QLabel(tr("Text weights"),textSearch);
        _textWeightsLineEdit = new QLineEdit(textSearch);

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(5, 5, 5, 5);
        vlayout->addLayout(defLangLayout);
        vlayout->addLayout(languageOverrideLayout);
        vlayout->addWidget(textWeights);
        vlayout->addWidget(_textWeightsLineEdit);
        textSearch->setLayout(vlayout);

        return textSearch;
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

    bool EditIndexDialog::isSparce()const
    {
        return _sparceCheckBox->checkState() == Qt::Checked;
    }

    QString EditIndexDialog::expireAfter() const
    {
        return _expireAfterLineEdit->text();
    }

    QString EditIndexDialog::defaultLanguage() const
    {
        return _defaultLanguageLineEdit->text();
    }

    QString EditIndexDialog::languageOverride() const
    {
        return _languageOverrideLineEdit->text();
    }

    QString EditIndexDialog::textWeights() const
    {
        return _textWeightsLineEdit->text();
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
