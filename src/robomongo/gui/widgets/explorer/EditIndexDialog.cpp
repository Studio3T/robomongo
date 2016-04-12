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
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/utils/QtUtils.h"
#include <robomongo/shell/bson/json.h>

namespace
{
    bool isValidJson(const QString &text)
    {
        bool result = false;
        if (!text.isEmpty()) {
            try {
                mongo::Robomongo::fromjson(text.toUtf8());
                result = true;
            }
            catch (const mongo::MsgAssertionException &) {
            }
        }
        return result;
    }

    QLabel *createHelpLabel(const QString &text, int marginLeft = 0, int marginTop = 0,
                            int marginRight = 0, int marginBottom = 0)
    {
        QLabel *helpLabel = new QLabel(text);
        helpLabel->setWordWrap(true);
        helpLabel->setContentsMargins(marginLeft, marginTop, marginRight, marginBottom);

        QPalette palette = helpLabel->palette();
        palette.setColor(QPalette::WindowText, QColor(110, 110, 110));
        helpLabel->setPalette(palette);

        return helpLabel;
    }

    Robomongo::FindFrame *createFindFrame(QWidget *parent = NULL, const QString &text = QString())
    {
        const QFont &textFont = Robomongo::GuiRegistry::instance().font();
        QsciLexerJavaScript *javaScriptLexer = new Robomongo::JSLexer(parent);
        javaScriptLexer->setFont(textFont);
        Robomongo::FindFrame *findFrame = new Robomongo::FindFrame(parent);
        findFrame->sciScintilla()->setLexer(javaScriptLexer);
        findFrame->sciScintilla()->setTabWidth(4);
        findFrame->sciScintilla()->setAppropriateBraceMatching();
        findFrame->sciScintilla()->setFont(textFont);
        findFrame->sciScintilla()->setStyleSheet("QFrame {background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
        findFrame->sciScintilla()->setText(text);
        findFrame->setMaximumHeight(120);
        return findFrame;
    }
}

namespace Robomongo
{
    EditIndexDialog::EditIndexDialog(const EnsureIndexInfo &info, const QString &databaseName, const QString &serverAdress, QWidget *parent)
        :BaseClass(parent), _info(info)
    {        
        setWindowTitle("Index Properties");
        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverAdress);
        Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), QtUtils::toQString(_info._collection.name()));
        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), databaseName);

        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setContentsMargins(2, 0, 5, 1);
        hlayout->setSpacing(0);
        hlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        hlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
        hlayout->addWidget(collectionIndicator, 0, Qt::AlignLeft);
        hlayout->addStretch(1);

        QTabWidget *mainTab = new QTabWidget(this);

        mainTab->addTab(createBasicTab(), tr("Basic"));
        mainTab->addTab(createAdvancedTab(), tr("Advanced"));
        mainTab->addTab(createTextSearchTab(), tr("Text Search"));
        mainTab->setTabsClosable(false);

        QDialogButtonBox *buttonBox = new QDialogButtonBox (this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QVBoxLayout *vlayout = new QVBoxLayout;
        vlayout->addLayout(hlayout);
        vlayout->addSpacing(5);
        vlayout->addWidget(mainTab);
        vlayout->addWidget(buttonBox);

        setLayout(vlayout);
    }

    QWidget* EditIndexDialog::createBasicTab()
    {
        QWidget *basicTab = new QWidget(this);
        _nameLineEdit = new QLineEdit(QtUtils::toQString(_info._name), basicTab);
        _nameLineEdit->setFocus();
        _jsonText = createFindFrame(basicTab, QtUtils::toQString(_info._request));
        _uniqueCheckBox = new QCheckBox(tr("Unique"));
        _uniqueCheckBox->setChecked(_info._unique);
        _dropDuplicates = new QCheckBox(tr("Drop duplicates"), basicTab);
        _dropDuplicates->setChecked(_info._dropDups);
        uniqueStateChanged(_uniqueCheckBox->checkState());

        QLabel *nameHelpLabel = createHelpLabel(
            "Choose any name that will help you to identify this index.",
            0, -2, 0, 15);

        QLabel *keyHelpLabel = createHelpLabel(
            "Document that contains pairs with the name of the field or fields to index "
            "and order of the index. A 1 specifies ascending and a -1 specifies descending.",
            0, -2, 0, 20);

        QLabel *uniqueHelpLabel = createHelpLabel(
            "If set, creates a unique index so that the collection will not accept insertion "
            "of documents where the index key or keys match an existing value in the index.",
            20, -2, 0, 20);

        QLabel *dropDupsHelpLabel = createHelpLabel(
            "MongoDB cannot create a unique index on a field that has duplicate values. "
            "To force the creation of a unique index, you can specify the dropDups option, "
            "which will only index the first occurrence of a value for the key, and delete all subsequent values. ",
            20, -2, 0, 20);

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(new QLabel("Name:   "),       0, 0);
        layout->addWidget(_nameLineEdit,                0, 1);
        layout->addWidget(nameHelpLabel,                1, 1, Qt::AlignTop);
        layout->addWidget(new QLabel("Keys:   "),       2, 0, Qt::AlignTop);
        layout->addWidget(_jsonText,                    2, 1, Qt::AlignTop);
        layout->addWidget(keyHelpLabel,                 3, 1, Qt::AlignTop);
        layout->addWidget(_uniqueCheckBox,              4, 0, 1, 2);
        layout->addWidget(uniqueHelpLabel,              5, 0, 1, 2, Qt::AlignTop);
        layout->addWidget(_dropDuplicates,              6, 0, 1, 2);
        layout->addWidget(dropDupsHelpLabel,            7, 0, 1, 2);
        layout->setAlignment(Qt::AlignTop);
        basicTab->setLayout(layout);
        VERIFY(connect(_uniqueCheckBox, SIGNAL(stateChanged(int)), this, SLOT(uniqueStateChanged(int))));
        return basicTab;
    }

    void EditIndexDialog::uniqueStateChanged(int value)
    {
        _dropDuplicates->setEnabled(value);
        if (!value) {
            _dropDuplicates->setCheckState(Qt::Unchecked);
        }
    }

    void EditIndexDialog::expireStateChanged(int value)
    {
        _expireAfterLineEdit->setEnabled(value);
        if (!value) {
            _expireAfterLineEdit->setText("");
        }
    }

    QWidget* EditIndexDialog::createAdvancedTab()
    {
        QWidget *advanced = new QWidget(this);

        _sparceCheckBox = new QCheckBox(tr("Sparse"), advanced);
        _sparceCheckBox->setChecked(_info._sparse);
        _backGroundCheckBox = new QCheckBox(tr("Create index in background"), advanced);
        _backGroundCheckBox->setChecked(_info._backGround);

        QHBoxLayout *expireLayout = new QHBoxLayout;
        _expireAfterLineEdit = new QLineEdit(advanced);
        _expireAfterLineEdit->setMaximumWidth(150);
        QRegExp rx("\\d+");
        _expireAfterLineEdit->setValidator(new QRegExpValidator(rx, this));

        QLabel *secLabel = new QLabel(tr("seconds"), advanced);
        expireLayout->addWidget(_expireAfterLineEdit);
        expireLayout->addWidget(secLabel);
        expireLayout->addStretch(1);

        QCheckBox *expireCheckBox = new QCheckBox(tr("Expire after"));
        expireCheckBox->setChecked(false);
        if (_info._ttl >= 0) {
            expireCheckBox->setChecked(true);
            _expireAfterLineEdit->setText(QString("%1").arg(_info._ttl));
        }
        expireStateChanged(expireCheckBox->checkState());
        VERIFY(connect(expireCheckBox, SIGNAL(stateChanged(int)), this, SLOT(expireStateChanged(int))));

        QLabel *sparseHelpLabel = createHelpLabel(
            "If set, the index only references documents with the specified field. "
            "These indexes use less space but behave differently in some situations (particularly sorts).",
            20, -2, 0, 20);

        QLabel *backgroundHelpLabel = createHelpLabel(
            "Builds the index in the background so that building an index does not block other database activities.",
            20, -2, 0, 20);

        QLabel *expireHelpLabel = createHelpLabel(
            "Specifies a <i>time to live</i>, in seconds, to control how long MongoDB retains documents in this collection",
            20, -2, 0, 20);

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(_sparceCheckBox,           0, 0, 1, 2);
        layout->addWidget(sparseHelpLabel,           1, 0, 1, 2);
        layout->addWidget(_backGroundCheckBox,       2, 0, 1, 2);
        layout->addWidget(backgroundHelpLabel,       3, 0, 1, 2);
        layout->addWidget(expireCheckBox,            4, 0);
        layout->addLayout(expireLayout,              4, 1);
        layout->addWidget(expireHelpLabel,           5, 0, 1, 2);
        layout->setAlignment(Qt::AlignTop);
        advanced->setLayout(layout);

        return advanced;
    }

    QWidget* EditIndexDialog::createTextSearchTab()
    {
        QWidget *textSearch = new QWidget(this);

        _defaultLanguageLineEdit = new QLineEdit(QtUtils::toQString(_info._defaultLanguage), textSearch);
        _languageOverrideLineEdit = new QLineEdit(QtUtils::toQString(_info._languageOverride), textSearch);
        _textWeightsLineEdit = createFindFrame(textSearch, QtUtils::toQString(_info._textWeights));

        QLabel *defaultLanguageHelpLabel = createHelpLabel(
            "For a <i>text</i> index, the language that determines the list of stop words and the rules for the stemmer and tokenizer. The default value is <b>english</b>",
            0, -2, 0, 20);

        QLabel *languageOverrideHelpLabel = createHelpLabel(
            "For a <i>text</i> index, specify the name of the field in the document that contains, for that document, the language to override the default language. The default value is <b>language</b>",
            0, -2, 0, 20);

        QLabel *textWeightsHelpLabel = createHelpLabel(
            "Document that contains field and weight pairs. The weight is a number ranging from 1 to 99,999 "
            "and denotes the significance of the field relative to the other indexed fields. ",
            0, -2, 0, 20);

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(new QLabel(tr("Default language:")),          0, 0);
        layout->addWidget(_defaultLanguageLineEdit,                     0, 1);
        layout->addWidget(defaultLanguageHelpLabel,                     1, 1);
        layout->addWidget(new QLabel(tr("Language override:")),         2, 0, Qt::AlignTop);
        layout->addWidget(_languageOverrideLineEdit,                    2, 1);
        layout->addWidget(languageOverrideHelpLabel,                    3, 1);
        layout->addWidget(new QLabel(tr("Text weights")),               4, 0, Qt::AlignTop);
        layout->addWidget(_textWeightsLineEdit,                         4, 1, Qt::AlignTop);
        layout->addWidget(textWeightsHelpLabel,                         5, 1);
        layout->setAlignment(Qt::AlignTop);
        textSearch->setLayout(layout);

        return textSearch;
    }

    EnsureIndexInfo EditIndexDialog::info() const
    {
        const QString &expAft = _expireAfterLineEdit->text();
        int expAftInt = _info._ttl;
        if (!expAft.isEmpty()) {
           expAftInt = _expireAfterLineEdit->text().toInt();
        }
        return EnsureIndexInfo(
            _info._collection,
            QtUtils::toStdString(_nameLineEdit->text()),
            QtUtils::toStdString(QString(" %1 ,{ name: %2 }")
                .arg(_jsonText->sciScintilla()->text())
                .arg(_nameLineEdit->text())),
            _uniqueCheckBox->checkState() == Qt::Checked,
            _backGroundCheckBox->checkState() == Qt::Checked,
            _dropDuplicates->checkState() == Qt::Checked,
            _sparceCheckBox->checkState() == Qt::Checked,
            expAftInt,
            QtUtils::toStdString(_defaultLanguageLineEdit->text()),
            QtUtils::toStdString(_languageOverrideLineEdit->text()),
            QtUtils::toStdString(_textWeightsLineEdit->sciScintilla()->text()));
    }

    void EditIndexDialog::accept()
    {
        if (isValidJson(_jsonText->sciScintilla()->text())) {
            const QString &weightText = _textWeightsLineEdit->sciScintilla()->text();
            if (!weightText.isEmpty() && !isValidJson(weightText)) {
                QMessageBox::warning(this, "Invalid json", "Please check json text.\n");
                _textWeightsLineEdit->setFocus();
                return ;
            }
            return BaseClass::accept();
        }
        else {
            QMessageBox::warning(this, "Invalid json", "Please check json text.\n");
            _jsonText->setFocus();
        }
    }
}
