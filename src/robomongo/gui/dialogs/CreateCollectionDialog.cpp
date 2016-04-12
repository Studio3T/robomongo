#include "robomongo/gui/dialogs/CreateCollectionDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QComboBox>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/shell/bson/json.h"

#include <mongo/bson/bsonobjbuilder.h>    

namespace Robomongo
{
    const QSize CreateCollectionDialog::dialogSize = QSize(300, 150);

    CreateCollectionDialog::CreateCollectionDialog(const QString &serverName, double dbVersion, const std::string& storageEngine, 
        const QString &database, const QString &collection, QWidget *parent) :
        QDialog(parent), _dbVersion(dbVersion), _storageEngine(storageEngine)
    {
        setWindowTitle("Create Collection"); 
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        //setFixedSize(dialogSize);
        setMinimumWidth(300);
        resize(520, 400);
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
        _tabWidget->addTab(createValidatorTab(), tr("Validator"));
        _tabWidget->addTab(createIndexOptionDefaultsTab(), tr("Index Option Defaults"));
        _tabWidget->setTabsClosable(false);
        VERIFY(connect(_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChangedSlot(int))));

        // Check db version and storage engine type to enable/disable UI options
        checkSupportedOptions();

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

    QString CreateCollectionDialog::jsonText(FindFrame* frame) const
    {
        return frame->sciScintilla()->text().trimmed();
    }

    const mongo::BSONObj& CreateCollectionDialog::getExtraOptions()
    {
        return _extraOptions;
    }

    void CreateCollectionDialog::setCursorPosition(int line, int column)
    {
        _activeFrame->sciScintilla()->setCursorPosition(line, column);
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
        if (_inputEdit->text().isEmpty() || !validate(_storageEngineFrame, _storageEngineObj)
            || !validate(_validatorFrame, _validatorObj) || !validateAllOptions())
            return;
        makeExtraOptionsObj();

        QDialog::accept();
    }

    void CreateCollectionDialog::cappedCheckBoxStateChanged(int newState)
    {
        _sizeInputEdit->setEnabled((Qt::Checked == static_cast<Qt::CheckState>(newState)));
        _maxDocNumberInputEdit->setEnabled((Qt::Checked == static_cast<Qt::CheckState>(newState)));
    }

    void CreateCollectionDialog::tabChangedSlot(int index)
    {
        if (0 == index){    // todo: make 0 to enum "Options Tab"
            _validateJsonButton->hide();
        }
        else{
            _validateJsonButton->show();
            if (1 == index){    
                _activeFrame = _storageEngineFrame;
                _activeObj = &_storageEngineObj;
            }
            else if (2 == index){
                _activeFrame = _validatorFrame;
                _activeObj = &_validatorObj;
            }
            else if (3 == index){
                _activeFrame = _indexOptionDefaultsFrame;
                _activeObj = &_indexOptionDefaultsObj;
            }
        }
    };

    bool CreateCollectionDialog::validate(FindFrame* frame, mongo::BSONObj& bsonObj, bool silentOnSuccess /* = true */)
    {
        try {
            bsonObj = mongo::Robomongo::fromjson(jsonText(frame).toStdString());
        }
        catch (const mongo::Robomongo::ParseMsgAssertionException &ex) {
            // v0.9
            QString message = QtUtils::toQString(ex.reason());
            int offset = ex.offset();

            int line = 0, pos = 0;
            frame->sciScintilla()->lineIndexFromPosition(offset, &line, &pos);
            frame->sciScintilla()->setCursorPosition(line, pos);

            int lineHeight = frame->sciScintilla()->lineLength(line);
            frame->sciScintilla()->fillIndicatorRange(line, pos, line, lineHeight, 0);

            message = QString("Unable to parse JSON:<br /> <b>%1</b>, at (%2, %3).")
                .arg(message).arg(line + 1).arg(pos + 1);

            QMessageBox::critical(NULL, "Parsing error", message);
            frame->setFocus();
            activateWindow();
            return false;
        }

        if (!silentOnSuccess) {
            QMessageBox::information(NULL, "Validation", "JSON is valid!");
            frame->setFocus();
            activateWindow();
        }

        return true;
    }

    bool CreateCollectionDialog::makeExtraOptionsObj()
    {
        mongo::BSONObjBuilder builder;
        if (_autoIndexCheckBox->isEnabled()) builder.append("autoIndexId", isCheckedAutoIndexid());
        if (_usePowerOfTwoSizeCheckBox->isEnabled()) builder.append("usePowerOf2Sizes", isCheckedUsePowerOfTwo());
        if (_noPaddingCheckBox->isEnabled()) builder.append("noPadding", isCheckedNoPadding());
        // todo: 1 and 2 to enum xxxTab
        if (_tabWidget->isTabEnabled(1) )
        {
            validate(_storageEngineFrame, _storageEngineObj);
            if(!_storageEngineObj.isEmpty()) builder.append("storageEngine", _storageEngineObj);
        }
        if (_tabWidget->isTabEnabled(2))    // todo: enum validatorTab
        {
            validate(_validatorFrame, _validatorObj);
            if (!_validatorObj.isEmpty())
            {
                builder.append("validator", _validatorObj);
                builder.append("validationLevel", _validatorLevelComboBox->currentText().toStdString());
                builder.append("validationAction", _validatorActionComboBox->currentText().toStdString());
            }
        }
        if (_tabWidget->isTabEnabled(3))
        {
            validate(_indexOptionDefaultsFrame, _indexOptionDefaultsObj);
            if (!_indexOptionDefaultsObj.isEmpty()) builder.append("indexOptionDefaults", _indexOptionDefaultsObj);
        }
        _extraOptions = builder.obj();
        // todo: return?
        return true;
    }

    bool CreateCollectionDialog::validateAllOptions() const
    {
        bool result(false);
        if (isCapped()){
            if (!(getSizeInputEditValue() > 0) || !(getMaxDocNumberInputEditValue() > 0)){
                QMessageBox::critical(NULL, "Error", "Invalid capped collection options");
                return false;
            }
            result = true;
        }
        // todo
        // ...
        result = true;
        return result;
    }

    void CreateCollectionDialog::checkSupportedOptions() const
    {
        if ("wiredTiger" == _storageEngine){     // todo: WiredTiger to be static const in other class
            _usePowerOfTwoSizeCheckBox->setDisabled(true);
            _noPaddingCheckBox->setDisabled(true);
            if (3.0 > _dbVersion){
                _tabWidget->setTabEnabled(1, false);    // todo: 1 to enum storageEngineTab
            }
        }
        else {
            _tabWidget->setTabEnabled(1, false);    // todo: 1 to enum storageEngineTab
        }

        if ("mmapv1" == _storageEngine){        // todo: what is the actual case sensitive string?
            if (3.0 <= _dbVersion){
                _usePowerOfTwoSizeCheckBox->setDisabled(true);
            }
            if (3.0 > _dbVersion){
                _noPaddingCheckBox->setDisabled(true);
            }
            if (3.0 <= _dbVersion){
                _usePowerOfTwoSizeCheckBox->setDisabled(true);
            }
        }
        else{
            _noPaddingCheckBox->setDisabled(true);
            _usePowerOfTwoSizeCheckBox->setDisabled(true);
        }

        if ("mmapv1" != _storageEngine && "wiredTiger" != _storageEngine){
            // ...
        }

        if (3.2 <= _dbVersion){
            _autoIndexCheckBox->setDisabled(true);
        }
        if (3.0 > _dbVersion){
            _tabWidget->setTabEnabled(1, false);    // todo: 1 to enum storageEngineTab
        }
        if (3.2 > _dbVersion){
            _tabWidget->setTabEnabled(2, false);    // todo: 2 to enum validatorTab
            _tabWidget->setTabEnabled(3, false);    // disable indexoptions.. tab
        }
    }

    void CreateCollectionDialog::onFrameTextChanged()
    {
        _activeFrame->sciScintilla()->clearIndicatorRange(0, 0, _activeFrame->sciScintilla()->lines(), 40, 0);
    }

    void CreateCollectionDialog::onValidateButtonClicked()
    {
        validate(_activeFrame, *_activeObj, false);
        makeExtraOptionsObj();      // todo: remove
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
        
        _storageEngineFrameLabel = new QLabel(tr("Enter the configuration for the storage engine: "));
        _storageEngineFrame = new FindFrame(this);
        configureFrameText(_storageEngineFrame);
        _storageEngineFrame->sciScintilla()->setText("{\n    \n}");
        // clear modification state after setting the content
        _storageEngineFrame->sciScintilla()->setModified(false);
        VERIFY(connect(_storageEngineFrame->sciScintilla(), SIGNAL(textChanged()), this, SLOT(onframeTextChanged())));

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(_storageEngineFrameLabel, 0, 0);
        layout->addWidget(_storageEngineFrame, 1, 0);
        layout->setAlignment(Qt::AlignTop);
        storageEngineTab->setLayout(layout);

        return storageEngineTab;
    }

    QWidget* CreateCollectionDialog::createValidatorTab()
    {
        QWidget *validatorEngineTab = new QWidget(this);

        _validatorLevelLabel = new QLabel(tr("Validation Level: "));
        _validatorLevelComboBox = new QComboBox();
        _validatorLevelComboBox->addItem("off");
        _validatorLevelComboBox->addItem("strict");
        _validatorLevelComboBox->addItem("moderate");
        _validatorLevelComboBox->setCurrentIndex(1);

        _validatorActionLabel = new QLabel(tr("Validation Action: "));
        _validatorActionComboBox = new QComboBox();
        _validatorActionComboBox->addItem("error");
        _validatorActionComboBox->addItem("warn");    // todo: warn or warning ??
        _validatorActionComboBox->setCurrentIndex(0);

        _validatorFrameLabel = new QLabel(tr("Enter the validator document for this collection: "));
        _validatorFrame = new FindFrame(this);
        configureFrameText(_validatorFrame);
        _validatorFrame->sciScintilla()->setText("{\n    \n}");
        // clear modification state after setting the content
        _validatorFrame->sciScintilla()->setModified(false);
        VERIFY(connect(_validatorFrame->sciScintilla(), SIGNAL(textChanged()), this, SLOT(onframeTextChanged())));

        QHBoxLayout *validationOptionslayout = new QHBoxLayout();
        validationOptionslayout->addWidget(_validatorLevelLabel);
        validationOptionslayout->addWidget(_validatorLevelComboBox, Qt::AlignLeft);
        validationOptionslayout->addWidget(_validatorActionLabel);
        validationOptionslayout->addWidget(_validatorActionComboBox, Qt::AlignLeft);

        QVBoxLayout *vlayout = new QVBoxLayout();
        vlayout->addWidget(_validatorFrameLabel);
        vlayout->addWidget(_validatorFrame);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addLayout(validationOptionslayout);
        layout->addLayout(vlayout);
        validatorEngineTab->setLayout(layout);

        return validatorEngineTab;
    }

    QWidget* CreateCollectionDialog::createIndexOptionDefaultsTab()
    {
        QWidget *indexOptionDefaultsTab = new QWidget(this);

        _indexOptionDefaultsFrameLabel = new QLabel(tr("Enter a default configuration for indexes when creating a collection: "));
        _indexOptionDefaultsFrame = new FindFrame(this);
        configureFrameText(_indexOptionDefaultsFrame);
        _indexOptionDefaultsFrame->sciScintilla()->setText("{\n    \n}");
        // clear modification state after setting the content
        _indexOptionDefaultsFrame->sciScintilla()->setModified(false);
        VERIFY(connect(_indexOptionDefaultsFrame->sciScintilla(), SIGNAL(textChanged()), this, SLOT(onframeTextChanged())));

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(_indexOptionDefaultsFrameLabel, 0, 0);
        layout->addWidget(_indexOptionDefaultsFrame, 1, 0);
        layout->setAlignment(Qt::AlignTop);
        indexOptionDefaultsTab->setLayout(layout);

        return indexOptionDefaultsTab;
    }

    void CreateCollectionDialog::configureFrameText(FindFrame* frame)
    {
        QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
        QFont font = GuiRegistry::instance().font();
        javaScriptLexer->setFont(font);
        frame->sciScintilla()->setBraceMatching(QsciScintilla::StrictBraceMatch);
        frame->sciScintilla()->setFont(font);
        frame->sciScintilla()->setPaper(QColor(255, 0, 0, 127));
        frame->sciScintilla()->setLexer(javaScriptLexer);
        frame->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_WORD);
        frame->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        frame->sciScintilla()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        frame->sciScintilla()->setStyleSheet("QFrame { background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
    }
}
