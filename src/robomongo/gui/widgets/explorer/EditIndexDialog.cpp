#include "robomongo/gui/widgets/explorer/EditIndexDialog.h"

#include <QTabWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QSpacerItem>
#include <QMessageBox>
#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/shell/db/json.h"

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
            catch (const mongo::ParseMsgAssertionException &) {

            }
        }
        return result;
    }
}

namespace Robomongo
{
    EditIndexDialog::EditIndexDialog(QWidget *parent,const EnsureIndexInfo &info,const QString &databaseName):BaseClass(parent),_info(info)
    {        
        Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), _info._collection.name());
        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), databaseName);

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
        _nameLineEdit = new QLineEdit(_info._name,basicTab);
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
        _jsonText->sciScintilla()->setText(_info._request);

        QHBoxLayout *checkBoxLayout = new QHBoxLayout;
        QSpacerItem *sp = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        _uniqueCheckBox = new QCheckBox(tr("Unique"),basicTab);
        _uniqueCheckBox->setChecked(_info._isUnique);
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
        _sparceCheckBox->setChecked(_info._isBackGround);
        _dropDuplicates = new QCheckBox(tr("Drop duplicates"),advanced);
        _dropDuplicates->setChecked(_info._isDropDuplicates);
        _backGroundCheckBox = new QCheckBox(tr("Create index in background"),advanced);
        _backGroundCheckBox->setChecked(_info._isBackGround);
        
        QHBoxLayout *expireLayout = new QHBoxLayout;
        QLabel *exLabel = new QLabel(tr("Expire after"));
        _expireAfterLineEdit = new QLineEdit(advanced);
        QRegExp rx("\\d+");
        _expireAfterLineEdit->setValidator( new QRegExpValidator(rx,this) );
        if(_info._expireAfter)
            _expireAfterLineEdit->setText(QString("%1").arg(_info._expireAfter));
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
        QLabel *defaultLanguage = new QLabel(tr("Default language:"),textSearch);
         _defaultLanguageLineEdit = new QLineEdit(_info._defaultLanguage,textSearch);
        defLangLayout->addWidget(defaultLanguage);
        defLangLayout->addWidget(_defaultLanguageLineEdit);

        QHBoxLayout *languageOverrideLayout = new QHBoxLayout;
        QLabel *languageOverrideLabel = new QLabel(tr("Language override:"),textSearch);
        _languageOverrideLineEdit = new QLineEdit(_info._languageOverride,textSearch);
        languageOverrideLayout->addWidget(languageOverrideLabel);
        languageOverrideLayout->addWidget(_languageOverrideLineEdit);

        QLabel *textWeights = new QLabel(tr("Text weights"),textSearch);
        _textWeightsLineEdit = new QTextEdit(_info._textWeights,textSearch);

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->setContentsMargins(5, 5, 5, 5);
        vlayout->addLayout(defLangLayout);
        vlayout->addLayout(languageOverrideLayout);
        vlayout->addWidget(textWeights);
        vlayout->addWidget(_textWeightsLineEdit);
        textSearch->setLayout(vlayout);

        return textSearch;
    }

    EnsureIndexInfo EditIndexDialog::info() const
    {
        return EnsureIndexInfo(_info._collection,_nameLineEdit->text(),QString(" %1 ,{ name: %2 }").arg(_jsonText->sciScintilla()->text()).arg(_nameLineEdit->text()),
             _uniqueCheckBox->checkState() == Qt::Checked,_backGroundCheckBox->checkState() == Qt::Checked,
             _dropDuplicates->checkState() == Qt::Checked,_sparceCheckBox->checkState() == Qt::Checked,
             _expireAfterLineEdit->text().toInt(),_defaultLanguageLineEdit->text(),
             _languageOverrideLineEdit->text(),_textWeightsLineEdit->toPlainText());
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
}
