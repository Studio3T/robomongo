#include "robomongo/gui/dialogs/CreateCollectionDialog.h"

#include <mongo/bson/bsonobjbuilder.h>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qsciscintilla.h>
#include <QVBoxLayout>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/shell/bson/json.h"

namespace Robomongo
{
    enum
    {
        OPTIONS_TAB                 = 0,
        STORAGE_ENGINE_TAB          = 1,
        VALIDATOR_TAB               = 2,
        INDEX_OPTION_DEFAULTS_TAB   = 3,
    };

    const QString CreateCollectionDialog::STORAGE_ENGINE_TAB_HINT = "Option available for WiredTiger storage engine only and database version 3.0 and higher.";
    const QString CreateCollectionDialog::VALIDATOR_TAB_HINT = "Option available for database version 3.2 and higher.";
    const QString CreateCollectionDialog::INDEX_OPTION_DEFAULTS_TAB_HINT = "Option available for database version 3.2 and higher.";
    const QString CreateCollectionDialog::NO_PADDING_HINT = "Option available for MMAPv1 storage engine only and database version 3.0 and higher.";
    const QString CreateCollectionDialog::USE_POWEROFTWO_HINT = "Option available for MMAPv1 storage engine only and deprecated since database version 3.0";
    const QString CreateCollectionDialog::AUTO_INDEXID_HINT = "Option deprecated since database version 3.2";

    CreateCollectionDialog::CreateCollectionDialog(const QString &serverName, const float dbVersion, const std::string& storageEngine, 
        const QString &database, const QString &collection, QWidget *parent) :
        QDialog(parent), _dbVersion(dbVersion), _storageEngine(storageEngine), 
        _activeFrame(nullptr), _activeObj(&_storageEngineObj)
    {
        setWindowTitle(tr("Create Collection"));
        setMinimumSize(300,150);
        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);

        QFrame *hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        _inputEdit = new QLineEdit();
        _inputLabel = new QLabel(tr("Collection Name:"));
        _inputEdit->setMaxLength(60);

        _buttonBox = new QDialogButtonBox(this);
        _buttonBox->setOrientation(Qt::Horizontal);
        _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        _buttonBox->button(QDialogButtonBox::Save)->setText(tr("C&reate"));
        _buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
        VERIFY(connect(_buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject())));
        VERIFY(connect(_inputEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableCreateButton(const QString &))));

        _validateJsonButton = new QPushButton(tr("Validate JSON"));
        _validateJsonButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation));
        _validateJsonButton->hide();
        VERIFY(connect(_validateJsonButton, SIGNAL(clicked()), this, SLOT(onValidateButtonClicked())));

        // Create advanced options tabs
        _advancedOptions = new QTabWidget(this);
        _advancedOptions->addTab(createOptionsTab(), tr("Options"));
        _advancedOptions->addTab(createStorageEngineTab(), tr("Storage Engine"));
        _advancedOptions->addTab(createValidatorTab(), tr("Validator"));
        _advancedOptions->addTab(createIndexOptionDefaultsTab(), tr("Index Option Defaults"));
        _advancedOptions->setTabsClosable(false);
        VERIFY(connect(_advancedOptions, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int))));
        // Create Advanced Button
        _advancedButton = new QPushButton(tr("Advanced"));
        _advancedButton->setCheckable(true);
        VERIFY(connect(_advancedButton, SIGNAL(toggled(bool)), this, SLOT(onAdvancedButtonToggled(bool))));

        // Check mongodb version and storage engine type to enable/disable UI options
        disableUnsupportedOptions();

        QHBoxLayout *buttomHlayout = new QHBoxLayout();
        buttomHlayout->addWidget(_validateJsonButton);
        buttomHlayout->addStretch(1);
        buttomHlayout->addWidget(_buttonBox);
        buttomHlayout->addWidget(_advancedButton);

        QHBoxLayout *vlayout = new QHBoxLayout();
        if (!serverName.isEmpty())
            vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        if (!database.isEmpty())
            vlayout->addWidget(createDatabaseIndicator(database), 0, Qt::AlignLeft);
        if (!collection.isEmpty())
            vlayout->addWidget(createCollectionIndicator(collection), 0, Qt::AlignLeft);
        vlayout->addStretch(1);

        QVBoxLayout *namelayout = new QVBoxLayout();
        namelayout->setContentsMargins(0, 7, 0, 7);
        namelayout->addWidget(_inputLabel);
        namelayout->addWidget(_inputEdit);
        
        QVBoxLayout *mainLayout = new QVBoxLayout();
        mainLayout->addLayout(vlayout);
        mainLayout->addWidget(hline);
        mainLayout->addLayout(namelayout);
        mainLayout->addWidget(_advancedOptions);
        mainLayout->addLayout(buttomHlayout);
        mainLayout->setSizeConstraint(QLayout::SetMaximumSize);
        setLayout(mainLayout);

        _advancedOptions->hide();
        _inputEdit->setFocus();
    }

    const mongo::BSONObj CreateCollectionDialog::getExtraOptions() const
    {
        return _extraOptionsObj;
    }

    QString CreateCollectionDialog::getCollectionName() const
    {
        return _inputEdit->text();
    }

    bool CreateCollectionDialog::isCapped() const
    {
        return (_cappedCheckBox->checkState() == Qt::Checked);
    }

    long long CreateCollectionDialog::getSizeInputValue() const
    {
        return (_sizeInputEdit->text().toLongLong());
    }

    int CreateCollectionDialog::getMaxDocNumberInputValue() const
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
        if ( _inputEdit->text().isEmpty() 
            || !validateOptionDependencies()
            || !validate(_storageEngineFrame, _storageEngineObj)
            || !validate(_validatorFrame, _validatorObj) 
            || !validate(_indexOptionDefaultsFrame, _indexOptionDefaultsObj))
        {
            return;
        }
        
        makeExtraOptionsObj();
        saveWindowSettings();

        QDialog::accept();
    }

    void CreateCollectionDialog::reject()
    {
        saveWindowSettings();
        QDialog::reject();
    }

    void CreateCollectionDialog::closeEvent(QCloseEvent *event)
    {
        if (_advancedOptions->isVisible())
        {
            saveWindowSettings();
        }
        QWidget::closeEvent(event);
    }

    void CreateCollectionDialog::onFrameTextChanged()
    {
        _activeFrame->sciScintilla()->clearIndicatorRange(0, 0, _activeFrame->sciScintilla()->lines(), 40, 0);
    }

    void CreateCollectionDialog::onValidateButtonClicked()
    {
        validate(_activeFrame, *_activeObj, false);
    }

    void CreateCollectionDialog::enableCreateButton(const QString &text)
    {
        _buttonBox->button(QDialogButtonBox::Save)->setEnabled(!text.isEmpty());
    }

    void CreateCollectionDialog::onCappedCheckBoxChanged(int newState)
    {
        _sizeInputEdit->setEnabled((Qt::Checked == static_cast<Qt::CheckState>(newState)));
        _maxDocNumberInputEdit->setEnabled((Qt::Checked == static_cast<Qt::CheckState>(newState)));
    }

    void CreateCollectionDialog::onTabChanged(int index)
    {
        if (OPTIONS_TAB == index) {    
            _validateJsonButton->hide();
        }
        else {
            _validateJsonButton->show();
            if (STORAGE_ENGINE_TAB == index) {
                _activeFrame = _storageEngineFrame;
                _activeObj = &_storageEngineObj;
            }
            else if (VALIDATOR_TAB == index) {
                _activeFrame = _validatorFrame;
                _activeObj = &_validatorObj;
            }
            else if (INDEX_OPTION_DEFAULTS_TAB == index) {
                _activeFrame = _indexOptionDefaultsFrame;
                _activeObj = &_indexOptionDefaultsObj;
            }
        }
    };

    void CreateCollectionDialog::onAdvancedButtonToggled(bool state)
    {
        _advancedOptions->setVisible(state);
        if (state)  // resize, dialog is expanding
        {
            setMinimumSize(560, 440);
            _validateJsonButton->setHidden(_advancedOptions->currentIndex() == OPTIONS_TAB);
            QSettings settings("3T", "Robomongo");
            if (settings.contains("CreateCollectionDialog/size"))
            {
                restoreWindowSettings();
            }
            else
            {
                resize(560, 440);
                adjustSize();
            }
        }
        else        // resize, dialog is shrinking
        {
            saveWindowSettings();       // save expanded geometry first
            _validateJsonButton->hide();
            setFixedSize(300, 150);
            adjustSize();
        }
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
        QWidget *optionsTab = new QWidget(this);

        _cappedCheckBox = new QCheckBox(tr("Create capped collection"), optionsTab);
        _sizeInputLabel = new QLabel(tr("Maximum size in bytes: "));
        _sizeInputLabel->setContentsMargins(22, 0, 0, 0);
        _sizeInputEdit = new QLineEdit();
        _sizeInputEdit->setEnabled(false);
        _maxDocNumberInputLabel = new QLabel(tr("Maximum number of documents: "));
        _maxDocNumberInputLabel->setContentsMargins(22, 0, 0, 0);
        _maxDocNumberInputEdit = new QLineEdit();
        _maxDocNumberInputEdit->setEnabled(false);
        _autoIndexCheckBox = new QCheckBox(tr("Auto index _id"), optionsTab);
        _autoIndexCheckBox->setChecked(true);
        _usePowerOfTwoSizeCheckBox = new QCheckBox(tr("Use power-of-2 sizes"), optionsTab);
        // Note: For mongodb 2.6 does not have storageEngine string due to the fact that it uses MMAPV1 only.
        if (MongoDatabase::StorageEngineType::MMAPV1 == _storageEngine || "" == _storageEngine)
        {
            _usePowerOfTwoSizeCheckBox->setChecked(true);
        }
        _noPaddingCheckBox = new QCheckBox(tr("No Padding"), optionsTab);

        VERIFY(connect(_cappedCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(onCappedCheckBoxChanged(int))));

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(_cappedCheckBox, 0, 0, 1, 2);
        layout->addWidget(_sizeInputLabel, 1, 0);
        layout->addWidget(_sizeInputEdit, 1, 1);
        layout->addWidget(_maxDocNumberInputLabel, 2, 0);
        layout->addWidget(_maxDocNumberInputEdit, 2, 1);
        layout->addWidget(_autoIndexCheckBox, 3, 0);
        layout->addWidget(_usePowerOfTwoSizeCheckBox, 4, 0);
        layout->addWidget(_noPaddingCheckBox, 5, 0);
        layout->setAlignment(Qt::AlignTop);
        optionsTab->setLayout(layout);

        return optionsTab;
    }

    QWidget* CreateCollectionDialog::createStorageEngineTab()
    {
        QWidget *storageEngineTab = new QWidget(this);

        _storageEngineFrameLabel = new QLabel(tr("Enter the configuration for the storage engine: "));
        _storageEngineFrame = new JSONFrame(this);
        configureFrameText(_storageEngineFrame);
        _storageEngineFrame->sciScintilla()->setText("{\n    \n}");
        // clear modification state after setting the content
        _storageEngineFrame->sciScintilla()->setModified(false);
        VERIFY(connect(_storageEngineFrame->sciScintilla(), SIGNAL(textChanged()),
            this, SLOT(onFrameTextChanged())));

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
        _validatorLevelComboBox->addItem(tr("off"));
        _validatorLevelComboBox->addItem(tr("strict"));
        _validatorLevelComboBox->addItem(tr("moderate"));
        _validatorLevelComboBox->setCurrentIndex(1);

        _validatorActionLabel = new QLabel(tr("Validation Action: "));
        _validatorActionComboBox = new QComboBox();
        _validatorActionComboBox->addItem(tr("error"));
        _validatorActionComboBox->addItem(tr("warn"));
        _validatorActionComboBox->setCurrentIndex(0);

        _validatorFrameLabel = new QLabel(tr("Enter the validator document for this collection: "));
        _validatorFrame = new JSONFrame(this);
        configureFrameText(_validatorFrame);
        _validatorFrame->sciScintilla()->setText("{\n    \n}");
        // clear modification state after setting the content
        _validatorFrame->sciScintilla()->setModified(false);
        VERIFY(connect(_validatorFrame->sciScintilla(), SIGNAL(textChanged()),
            this, SLOT(onFrameTextChanged())));

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
        _indexOptionDefaultsFrame = new JSONFrame(this);
        configureFrameText(_indexOptionDefaultsFrame);
        _indexOptionDefaultsFrame->sciScintilla()->setText("{\n    \n}");
        // clear modification state after setting the content
        _indexOptionDefaultsFrame->sciScintilla()->setModified(false);
        VERIFY(connect(_indexOptionDefaultsFrame->sciScintilla(), SIGNAL(textChanged()),
            this, SLOT(onFrameTextChanged())));

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(_indexOptionDefaultsFrameLabel, 0, 0);
        layout->addWidget(_indexOptionDefaultsFrame, 1, 0);
        layout->setAlignment(Qt::AlignTop);
        indexOptionDefaultsTab->setLayout(layout);

        return indexOptionDefaultsTab;
    }

    void CreateCollectionDialog::saveWindowSettings() const
    {
        QSettings settings("3T", "Robomongo");
        settings.setValue("CreateCollectionDialog/size", size());
    }

    void CreateCollectionDialog::restoreWindowSettings()
    {
        QSettings settings("3T", "Robomongo");
        resize(settings.value("CreateCollectionDialog/size").toSize());
    }

    void CreateCollectionDialog::configureFrameText(JSONFrame* frame)
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

    bool CreateCollectionDialog::validate(JSONFrame* frame, mongo::BSONObj& bsonObj, bool silentOnSuccess /* = true */)
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

            message = QString(tr("Unable to parse JSON:<br /> <b>%1</b>, at (%2, %3)."))
                .arg(message).arg(line + 1).arg(pos + 1);

            QMessageBox::critical(NULL, tr("Parsing error"), message);
            frame->setFocus();
            activateWindow();
            return false;
        }

        if (!silentOnSuccess) {
            QMessageBox::information(NULL, tr("Validation"), tr("JSON is valid!"));
            frame->setFocus();
            activateWindow();
        }

        return true;
    }

    void CreateCollectionDialog::makeExtraOptionsObj()
    {
        mongo::BSONObjBuilder builder;

        if (_autoIndexCheckBox->isEnabled()) builder.append("autoIndexId", isCheckedAutoIndexid()); 
        if (_noPaddingCheckBox->isEnabled() && _usePowerOfTwoSizeCheckBox->isEnabled()) {
            builder.append("flags", (isCheckedNoPadding()*2 + isCheckedUsePowerOfTwo()*1));
        }
        else if (_noPaddingCheckBox->isEnabled()) {
            builder.append("flags", (isCheckedNoPadding()*2));
        }
        else if (_usePowerOfTwoSizeCheckBox->isEnabled()) {
            builder.append("flags", (isCheckedUsePowerOfTwo()*1));
        }
        if (_advancedOptions->isTabEnabled(STORAGE_ENGINE_TAB) ) {
            validate(_storageEngineFrame, _storageEngineObj);
            if(!_storageEngineObj.isEmpty()) builder.append("storageEngine", _storageEngineObj);
        }
        if (_advancedOptions->isTabEnabled(VALIDATOR_TAB)) {
            validate(_validatorFrame, _validatorObj);
            if (!_validatorObj.isEmpty()) {
                builder.append("validator", _validatorObj);
                builder.append("validationLevel", _validatorLevelComboBox->currentText().toStdString());
                builder.append("validationAction", _validatorActionComboBox->currentText().toStdString());
            }
        }
        if (_advancedOptions->isTabEnabled(INDEX_OPTION_DEFAULTS_TAB)) {
            validate(_indexOptionDefaultsFrame, _indexOptionDefaultsObj);
            if (!_indexOptionDefaultsObj.isEmpty()) builder.append("indexOptionDefaults", _indexOptionDefaultsObj);
        }
        // Complete and get resulting BSONObj
        _extraOptionsObj = builder.obj();
    }

    bool CreateCollectionDialog::validateOptionDependencies() const
    {
        bool result(false);
        // Validate capped options
        if (isCapped()) {
            if (!(getSizeInputValue() > 0)) {
                QMessageBox::critical(NULL, tr("Error"), tr("Maximum size is required for capped collections"));
                return false;
            }
            result = true;
        }
        else {
            result = true;
        }

        // Completed checking all dependendencies.
        return result;
    }
    
    void CreateCollectionDialog::disableUnsupportedOptions() const
    {

        // Handle WIRED_TIGER engine type
        if (MongoDatabase::StorageEngineType::WIRED_TIGER == _storageEngine) {
            disableOption(_usePowerOfTwoSizeCheckBox, USE_POWEROFTWO_HINT);
            disableOption(_noPaddingCheckBox, NO_PADDING_HINT);
            if (MongoDatabase::DBVersion::MONGODB_3_0 > _dbVersion) {
                disableTab(STORAGE_ENGINE_TAB, STORAGE_ENGINE_TAB_HINT);
            }
        }
        else {
            disableTab(STORAGE_ENGINE_TAB, STORAGE_ENGINE_TAB_HINT);
        }

        // Handle MMAPV1 engine type
        // Note: For mongodb 2.6 does not have storageEngine string due to the fact that it uses MMAPV1 only.
        if (MongoDatabase::StorageEngineType::MMAPV1 == _storageEngine || "" == _storageEngine) {
            if (MongoDatabase::DBVersion::MONGODB_3_0 < _dbVersion) {
                _usePowerOfTwoSizeCheckBox->hide();
            }
            if (MongoDatabase::DBVersion::MONGODB_3_0 > _dbVersion) {
                disableOption(_noPaddingCheckBox, NO_PADDING_HINT);
            }
        }

        if (MongoDatabase::DBVersion::MONGODB_3_2 < _dbVersion) {
            disableOption(_autoIndexCheckBox, AUTO_INDEXID_HINT);
        }

        if (MongoDatabase::DBVersion::MONGODB_3_0 > _dbVersion) {
            disableTab(STORAGE_ENGINE_TAB, STORAGE_ENGINE_TAB_HINT);
        }
        if (MongoDatabase::DBVersion::MONGODB_3_2 > _dbVersion) {
            disableTab(VALIDATOR_TAB, VALIDATOR_TAB_HINT);
            disableTab(INDEX_OPTION_DEFAULTS_TAB, INDEX_OPTION_DEFAULTS_TAB_HINT);
        }
    }

    void CreateCollectionDialog::disableOption(QWidget* option, const QString& hint) const
    {
        option->setDisabled(true);
        option->setToolTip(hint);
    }

    void CreateCollectionDialog::disableTab(int index, const QString& hint) const
    {
        _advancedOptions->setTabEnabled(index, false);
        _advancedOptions->setTabToolTip(index, hint);
    }

    void CreateCollectionDialog::setCursorPosition(int line, int column)
    {
        _activeFrame->sciScintilla()->setCursorPosition(line, column);
    }

    QString CreateCollectionDialog::jsonText(JSONFrame* frame) const
    {
        return frame->sciScintilla()->text().trimmed();
    }
}
