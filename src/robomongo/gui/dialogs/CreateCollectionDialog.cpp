#include "robomongo/gui/dialogs/CreateCollectionDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QMessageBox>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"

#include "robomongo/shell/bson/json.h"

namespace Robomongo
{
    const QSize CreateCollectionDialog::dialogSize = QSize(300, 150);

    CreateCollectionDialog::CreateCollectionDialog(const QString &serverName, const QString &database,
        const QString &collection, QWidget *parent) :
        QDialog(parent)
    {
        setWindowTitle("Create Collection"); 
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        //setFixedSize(dialogSize);
        setMinimumWidth(300);

        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);

        QFrame *hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        _inputEdit = new QLineEdit();
        _inputLabel = new QLabel("Database Name:");
        _inputEdit->setMaxLength(maxLenghtName);

        _buttonBox = new QDialogButtonBox(this);
        _buttonBox->setOrientation(Qt::Horizontal);
        _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        _buttonBox->button(QDialogButtonBox::Save)->setText("C&reate");
        _buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
        VERIFY(connect(_buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject())));
        VERIFY(connect(_inputEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableFindButton(const QString &))));

        _validateJsonButton = new QPushButton("Validate JSON"); // todo: need to change other UI accordingly
        _validateJsonButton->hide();
        VERIFY(connect(_validateJsonButton, SIGNAL(clicked()), this, SLOT(onValidateButtonClicked())));

        // Create tabs at last
        _tabWidget = new QTabWidget(this);
        _tabWidget->addTab(createOptionsTab(), tr("Options"));
        _tabWidget->addTab(createStorageEngineTab(), tr("Storage Engine"));
        //_tabWidget->addTab(createTextSearchTab(), tr("Validator"));
        _tabWidget->setTabsClosable(false);
        VERIFY(connect(_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChangedSlot(int))));

        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->addWidget(_validateJsonButton);
        hlayout->addStretch(1);
        hlayout->addWidget(_buttonBox);

        QHBoxLayout *vlayout = new QHBoxLayout();
        if (!serverName.isEmpty())
            vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        if (!database.isEmpty())
            vlayout->addWidget(createDatabaseIndicator(database), 0, Qt::AlignLeft);
        if (!collection.isEmpty())
            vlayout->addWidget(createCollectionIndicator(collection), 0, Qt::AlignLeft);

        QVBoxLayout *namelayout = new QVBoxLayout();
        namelayout->setContentsMargins(0, 7, 0, 7);
        namelayout->addWidget(_inputLabel);
        namelayout->addWidget(_inputEdit);
        
        QVBoxLayout *layout = new QVBoxLayout();
        layout->addLayout(vlayout);
        layout->addWidget(hline);
        layout->addLayout(namelayout);
        layout->addWidget(_tabWidget);
        layout->addLayout(hlayout);
        setLayout(layout);

        _inputEdit->setFocus();
    }

    QString CreateCollectionDialog::jsonText() const
    {
        return _frameText->sciScintilla()->text().trimmed();
    }

    void CreateCollectionDialog::setCursorPosition(int line, int column)
    {
        _frameText->sciScintilla()->setCursorPosition(line, column);
    }

    QString CreateCollectionDialog::getCollectionName() const
    {
        return _inputEdit->text();
    }

    void CreateCollectionDialog::setOkButtonText(const QString &text)
    {
        _buttonBox->button(QDialogButtonBox::Save)->setText(text);
    }

    void CreateCollectionDialog::setInputLabelText(const QString &text)
    {
        _inputLabel->setText(text);
    }

    void CreateCollectionDialog::setInputText(const QString &text)
    {
        _inputEdit->setText(text);
        _inputEdit->selectAll();
    }

    bool CreateCollectionDialog::isCapped() const
    {
        return (_cappedCheckBox->checkState() == Qt::Checked);
    }

    long long CreateCollectionDialog::getSizeInputEditValue() const
    {
        return (_sizeInputEdit->text().toLongLong());
    }
    
    int CreateCollectionDialog::getMaxDocNumberInputEditValue() const
    {
        return (_maxDocNumberInputEdit->text().toInt());
    }

    bool CreateCollectionDialog::isCheckedAutoIndexid() const
    {
        return (_autoIndexCheckBox->checkState() == Qt::Checked);
    }

    bool CreateCollectionDialog::isCheckedUsePowerOfTwo() const
    {
        return (_usePowerOfTwoSizeCheckBox->checkState() == Qt::Checked);
    }

    bool CreateCollectionDialog::isCheckedNoPadding() const
    {
        return (_noPaddingCheckBox->checkState() == Qt::Checked);
    }

    void CreateCollectionDialog::accept()
    {
        if (_inputEdit->text().isEmpty() && !validate())
            return;

        QDialog::accept();
    }

    void CreateCollectionDialog::cappedCheckBoxStateChanged(int newState)
    {
        _sizeInputEdit->setEnabled((Qt::Checked == static_cast<Qt::CheckState>(newState)));
        _maxDocNumberInputEdit->setEnabled((Qt::Checked == static_cast<Qt::CheckState>(newState)));
    }

    void CreateCollectionDialog::tabChangedSlot(int index)
    {
        if (0 == index){    // todo: make 0 to enum
            _validateJsonButton->hide();
        }
        else{
            _validateJsonButton->show();
        }
    };

    bool CreateCollectionDialog::validate(bool silentOnSuccess /* = true */)
    {
        QString text = jsonText();
        int len = 0;
        try {
            std::string textString = QtUtils::toStdString(text);
            const char *json = textString.c_str();
            int jsonLen = textString.length();
            int offset = 0;
            _obj.clear();
            while (offset != jsonLen)
            {
                mongo::BSONObj doc = mongo::Robomongo::fromjson(json + offset, &len);
                _obj.push_back(doc);
                offset += len;
            }
        }
        catch (const mongo::Robomongo::ParseMsgAssertionException &ex) {
            //            v0.9
            QString message = QtUtils::toQString(ex.reason());
            int offset = ex.offset();

            int line = 0, pos = 0;
            _frameText->sciScintilla()->lineIndexFromPosition(offset, &line, &pos);
            _frameText->sciScintilla()->setCursorPosition(line, pos);

            int lineHeight = _frameText->sciScintilla()->lineLength(line);
            _frameText->sciScintilla()->fillIndicatorRange(line, pos, line, lineHeight, 0);

            message = QString("Unable to parse JSON:<br /> <b>%1</b>, at (%2, %3).")
                .arg(message).arg(line + 1).arg(pos + 1);

            QMessageBox::critical(NULL, "Parsing error", message);
            _frameText->setFocus();
            activateWindow();
            return false;
        }

        if (!silentOnSuccess) {
            QMessageBox::information(NULL, "Validation", "JSON is valid!");
            _frameText->setFocus();
            activateWindow();
        }

        return true;
    }

    void CreateCollectionDialog::onFrameTextChanged()
    {
        _frameText->sciScintilla()->clearIndicatorRange(0, 0, _frameText->sciScintilla()->lines(), 40, 0);
    }

    void CreateCollectionDialog::onValidateButtonClicked()
    {
        validate(false);  
    }

    void CreateCollectionDialog::enableFindButton(const QString &text)
    {
        _buttonBox->button(QDialogButtonBox::Save)->setEnabled(!text.isEmpty());
    }

    Indicator *CreateCollectionDialog::createDatabaseIndicator(const QString &database)
    {
        return new Indicator(GuiRegistry::instance().databaseIcon(), database);
    }

    Indicator *CreateCollectionDialog::createCollectionIndicator(const QString &collection)
    {
        return new Indicator(GuiRegistry::instance().collectionIcon(), collection);
    }

    QWidget* CreateCollectionDialog::createOptionsTab()
    {
        QWidget *options = new QWidget(this);   // todo: rename optionsTab

        _cappedCheckBox = new QCheckBox(tr("Create capped collection"), options);
        _sizeInputLabel = new QLabel(tr("Maximum size in bytes: "));
        _sizeInputLabel->setContentsMargins(22, 0, 0, 0);
        _sizeInputEdit = new QLineEdit();
        _sizeInputEdit->setEnabled(false);
        _maxDocNumberInputLabel = new QLabel(tr("Maximum number of documents: "));
        _maxDocNumberInputLabel->setContentsMargins(22, 0, 0, 0);
        _maxDocNumberInputEdit = new QLineEdit();
        _maxDocNumberInputEdit->setEnabled(false);
        _autoIndexCheckBox = new QCheckBox(tr("Auto index _id"), options);
        _autoIndexCheckBox->setChecked(true);
        _usePowerOfTwoSizeCheckBox = new QCheckBox(tr("Use power-of-2 sizes"), options);
        _noPaddingCheckBox = new QCheckBox(tr("No Padding"), options);

        VERIFY(connect(_cappedCheckBox, SIGNAL(stateChanged(int)), this, SLOT(cappedCheckBoxStateChanged(int))));

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(_cappedCheckBox,              0, 0, 1, 2);
        layout->addWidget(_sizeInputLabel,              1, 0);
        layout->addWidget(_sizeInputEdit,               1, 1);
        layout->addWidget(_maxDocNumberInputLabel,      2, 0);
        layout->addWidget(_maxDocNumberInputEdit,       2, 1);
        layout->addWidget(_autoIndexCheckBox,           3, 0);
        layout->addWidget(_usePowerOfTwoSizeCheckBox,   4, 0);
        layout->addWidget(_noPaddingCheckBox,           5, 0);
        layout->setAlignment(Qt::AlignTop);
        options->setLayout(layout);

        return options;
    }

    QWidget* CreateCollectionDialog::createStorageEngineTab()
    {
        QWidget *storageEngineTab = new QWidget(this);
        
        _frameLabel = new QLabel(tr("Enter the configuration for the storage engine: "));
        _frameText = new FindFrame(this);
        configureQueryText();
        _frameText->sciScintilla()->setText("{\n    \n}");
        // clear modification state after setting the content
        _frameText->sciScintilla()->setModified(false);
        VERIFY(connect(_frameText->sciScintilla(), SIGNAL(textChanged()), this, SLOT(onframeTextChanged())));

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(_frameLabel, 0, 0);
        layout->addWidget(_frameText, 1, 0);
        layout->setAlignment(Qt::AlignTop);
        storageEngineTab->setLayout(layout);

        return storageEngineTab;
    }

    void CreateCollectionDialog::configureQueryText()
    {
        QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
        QFont font = GuiRegistry::instance().font();
        javaScriptLexer->setFont(font);
        _frameText->sciScintilla()->setBraceMatching(QsciScintilla::StrictBraceMatch);
        _frameText->sciScintilla()->setFont(font);
        _frameText->sciScintilla()->setPaper(QColor(255, 0, 0, 127));
        _frameText->sciScintilla()->setLexer(javaScriptLexer);
        _frameText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_WORD);
        _frameText->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        _frameText->sciScintilla()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        _frameText->sciScintilla()->setStyleSheet("QFrame { background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
    }
}
