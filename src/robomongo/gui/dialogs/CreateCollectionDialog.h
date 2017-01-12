#pragma once

#include <mongo/bson/bsonobj.h>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QDialogButtonBox;
class QLineEdit;
class QTabWidget;
class QCheckBox;
class QComboBox;
QT_END_NAMESPACE
class QsciScintilla;

namespace Robomongo
{
    class FindFrame;
    class Indicator;

    /**
    * @brief This Dialog allows to create collection with selective options.
    */
    class CreateCollectionDialog : public QDialog
    {
        Q_OBJECT

    public:
        using JSONFrame = FindFrame;
        
        /**
        * @brief Construct dialog with parameters
        * @param serverName: Database name
        * @param dbVersion: Database version
        * @param storageEngine: Storage engine type
        * @param database: Name of the database
        * @param collection: Name of the collection
        */
        explicit CreateCollectionDialog(const QString &serverName, const float dbVersion, const std::string& storageEngine, 
            const QString &database = QString(), const QString &collection = QString(), QWidget *parent = 0);

        /**
        * @brief Used if dialog exec() method returns QDialog::Accepted 
        * @return Extra options as BSONObj
        */
        const mongo::BSONObj getExtraOptions() const;

        /**
        * @return Collection name
        */
        QString getCollectionName() const;

        /**
        * @return true if capped option checked, false otherwise
        */
        bool isCapped() const;

        /**
        * @return Size option input
        */
        long long getSizeInputValue() const;

        /**
        * @return Max number of documents option input
        */
        int getMaxDocNumberInputValue() const;

        /**
        * @return true if auto index_id option checked, false otherwise
        */
        bool isCheckedAutoIndexid() const;
        
        /**
        * @return true if use power-of-2 sizes option checked, false otherwise
        */
        bool isCheckedUsePowerOfTwo() const;
        
        /**
        * @return true if no padding option checked, false otherwise
        */
        bool isCheckedNoPadding() const;

    public Q_SLOTS:
        /**
        * @brief Validate all input fields and make extraOptions object if all valid when create button clicked.
        */
        void accept() override;

        /**
        * @brief Called when "Cancel" button clicked.
        */
        void reject() override;

    protected:
        /**
        * @brief Reimplementing closeEvent in order to do implement functionalities before close this dialog.
        */
        void closeEvent(QCloseEvent *event) override;

    private Q_SLOTS:
        /**
        * @brief Set indicator position when active JSON frame text changed
        */
        void onFrameTextChanged();

        /**
        * @brief Validate active JSON frame text when validate button clicked
        */
        void onValidateButtonClicked();

        /**
        * @brief Enable create button if input edit has text
        * @param text : string from _inputEdit widget
        */
        void enableCreateButton(const QString &text);

        /**
        * @brief Enable/disable capped options when capped checkbox state changed
        * @param newState : New state of capped checkbox
        */
        void onCappedCheckBoxChanged(int newState);

        /**
        * @brief Show/hide validate JSON button, update active JSON frame and object according to active tab
        * @param Index of active tab (i.e. index = 0 for the first tab)
        */
        void onTabChanged(int index);

        /**
        * @brief Show/hide advanced options menu (_tabwidget) when advancedButton toggled.
        * @param state : true if dialog is expanding, false if shrinking.
        */
        void onAdvancedButtonToggled(bool state);

    private:
        /**
        * @brief Tool tip strings for disabled options and tabs
        */
        static const QString STORAGE_ENGINE_TAB_HINT;
        static const QString VALIDATOR_TAB_HINT;
        static const QString INDEX_OPTION_DEFAULTS_TAB_HINT;
        static const QString NO_PADDING_HINT;
        static const QString USE_POWEROFTWO_HINT;
        static const QString AUTO_INDEXID_HINT;

        /**
        * @brief Create database indicator widget
        * @param database : Name of the database
        * @return : Pointer to newly created Indicator
        */
        Indicator* createDatabaseIndicator(const QString &database);

        /**
        * @brief Create collection indicator widget
        * @param database : Name of the collection
        * @return : Pointer to newly created Indicator
        */
        Indicator* createCollectionIndicator(const QString &collection);

        /**
        * @brief Creators for the tabs
        * @return : Pointer to newly created tab
        */
        QWidget* createOptionsTab();
        QWidget* createStorageEngineTab();
        QWidget* createValidatorTab();
        QWidget* createIndexOptionDefaultsTab();

        /**
        * @brief Save windows settings into system registry
        */
        void saveWindowSettings() const;
        /**
        * @brief Restore windows settings from system registry
        */
        void restoreWindowSettings();

        /**
        * @brief Do initial configuration of given JSON frame.
        * @param frame : Pointer to newly created tab
        */
        void configureFrameText(JSONFrame* frame);

        /**
        * @brief Build extraOptions BSON Object
        */ 
        void makeExtraOptionsObj();
        
        /**
        * @brief Validate dependencies and requirements between all inputs
        * @return : true if all options valid, false at least one dependency not met
        */
        bool validateOptionDependencies() const;

        /**
        * @brief Disable options if they are not supported by database version or storage engine type
        */
        void disableUnsupportedOptions() const;

        /**
        * @brief Disable widget if unsupported
        * @param option : Widget to disable
        * @param hint : Hint string to be shown
        */
        void disableOption(QWidget* option, const QString& hint) const;
        
        /**
        * @brief Disable tab if unsupported
        * @param index : Tab index
        * @param hint : Hint string to be shown
        */
        void disableTab(int index, const QString& hint) const;

        /**
        * @brief Set cursor position of the active JSON frame
        * @param line :  Line number 
        * @param column : Column number
        */
        void setCursorPosition(int line, int column);

        /**
        * @return : String from given JSON frame
        * @param frame : Pointer to JSON frame
        */
        QString jsonText(JSONFrame* frame) const;

        /**
        * @brief Do JSON validation/creation for given frame text
        * @param frame : Pointer to JSON frame
        * @param bsonObj : BSON object to be validated
        * @param silentOnSuccess : true for silent validation, false for message box on validation
        * @return true if text is valid JSON, false otherwise
        */
        bool validate(JSONFrame* frame, mongo::BSONObj& bsonObj, bool silentOnSuccess = true);

        /**
        * @brief Main Window 
        */
        QLineEdit *_inputEdit;
        QLabel *_inputLabel;
        QTabWidget *_advancedOptions;
        QPushButton *_validateJsonButton;
        QDialogButtonBox *_buttonBox;
        QPushButton *_advancedButton;

        /**
        * @brief Options Tab
        */
        QCheckBox *_cappedCheckBox;
        QLabel *_sizeInputLabel;
        QLineEdit *_sizeInputEdit;
        QLabel *_maxDocNumberInputLabel;
        QLineEdit *_maxDocNumberInputEdit;
        QCheckBox *_autoIndexCheckBox;
        QCheckBox *_usePowerOfTwoSizeCheckBox;
        QCheckBox *_noPaddingCheckBox;
        /**
        * @brief Storage Engine Tab
        */
        JSONFrame *_storageEngineFrame;
        QLabel * _storageEngineFrameLabel;
        /**
        * @brief Validator Tab
        */
        QLabel * _validatorLevelLabel;
        QComboBox * _validatorLevelComboBox;
        QLabel * _validatorActionLabel;
        QComboBox * _validatorActionComboBox;
        JSONFrame *_validatorFrame;
        QLabel * _validatorFrameLabel;
        /**
        * @brief Index Options Defaults Tab
        */
        JSONFrame *_indexOptionDefaultsFrame;
        QLabel * _indexOptionDefaultsFrameLabel;

        /**
        * @brief Pointer to active tab's frame
        */
        JSONFrame *_activeFrame;         

        /**
        * @brief Pointer to active tab's BSON obj.
        */
        mongo::BSONObj *_activeObj;

        /**
        * @brief Main BSON obj. with all extra options
        */
        mongo::BSONObj _extraOptionsObj;

        /**
        * @brief Sub objects for each JSON tab to create extraOptions object finally.
        */
        mongo::BSONObj _storageEngineObj;
        mongo::BSONObj _validatorObj;
        mongo::BSONObj _indexOptionDefaultsObj;

        /**
        * @brief Database version
        */
        const float _dbVersion;

        /**
        * @brief Storage engine type
        */
        const std::string _storageEngine;
    };
}
