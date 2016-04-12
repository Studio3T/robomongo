#pragma once

#include <QDialog>
#include <mongo/bson/bsonobj.h>

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
    class FindFrame;    // todo: new class or typedef i.e. JSONFrame
    class Indicator;

    class CreateCollectionDialog : public QDialog
    {
        Q_OBJECT

    public:
        typedef std::vector<mongo::BSONObj> ReturnType;     // todo: public access?

        explicit CreateCollectionDialog(const QString &serverName, double dbVersion, const std::string& storageEngine, 
            const QString &database = QString(), const QString &collection = QString(), QWidget *parent = 0);


        QString jsonText(FindFrame* frame) const;   // todo: private
        /**
        * @brief Use returned BSONObj only if Dialog exec() method returns QDialog::Accepted
        */
        const mongo::BSONObj& getStorageEngineBsonObj() const { return _storageEngineObj; }    
        const mongo::BSONObj& getValidatorBsonObj() const { return _validatorObj; }            
        const mongo::BSONObj& getExtraOptions();
        void setCursorPosition(int line, int column);   // todo: private

        QString getCollectionName() const;
        void setOkButtonText(const QString &text);
        void setInputLabelText(const QString &text);
        void setInputText(const QString &text);

        // Options Tab
        bool isCapped() const;
        long long getSizeInputEditValue() const;    // todo: shorten name
        int getMaxDocNumberInputEditValue() const;
        bool isCheckedAutoIndexid() const;
        bool isCheckedUsePowerOfTwo() const;
        bool isCheckedNoPadding() const;

        enum { maxLenghtName = 60 };
        const static QSize dialogSize;

        public Q_SLOTS:
        virtual void accept();
        void cappedCheckBoxStateChanged(int newState);
        void tabChangedSlot(int index);
        // todo: const frame and bsonobj
        bool validate(FindFrame* frame, mongo::BSONObj& bsonObj, bool silentOnSuccess = true);

        private Q_SLOTS:
        void onFrameTextChanged();
        void onValidateButtonClicked();
        void enableFindButton(const QString &text);

    private:
        Indicator *createDatabaseIndicator(const QString &database);
        Indicator *createCollectionIndicator(const QString &collection);
        QWidget *createOptionsTab();
        QWidget *createStorageEngineTab();
        QWidget *createValidatorTab();
        QWidget *createIndexOptionDefaultsTab();
        void configureFrameText(FindFrame* frame);
        bool makeExtraOptionsObj();
        bool validateAllOptions() const;
        void checkSupportedOptions() const;

        // Main Frame
        QLineEdit *_inputEdit;
        QLabel *_inputLabel;
        QTabWidget *_tabWidget;
        QPushButton *_validateJsonButton;
        QDialogButtonBox *_buttonBox;
        // Options Tab
        QCheckBox *_cappedCheckBox;
        QLabel *_sizeInputLabel;
        QLineEdit *_sizeInputEdit;
        QLabel *_maxDocNumberInputLabel;
        QLineEdit *_maxDocNumberInputEdit;
        QCheckBox *_autoIndexCheckBox;
        QCheckBox *_usePowerOfTwoSizeCheckBox;
        QCheckBox *_noPaddingCheckBox;
        // Storage Engine Tab
        FindFrame *_storageEngineFrame;
        QLabel * _storageEngineFrameLabel;
        // Validator Tab
        QLabel * _validatorLevelLabel;
        QComboBox * _validatorLevelComboBox;
        QLabel * _validatorActionLabel;
        QComboBox * _validatorActionComboBox;
        FindFrame *_validatorFrame;
        QLabel * _validatorFrameLabel;
        // Index Options Defaults Tab
        FindFrame *_indexOptionDefaultsFrame;
        QLabel * _indexOptionDefaultsFrameLabel;

        FindFrame *_activeFrame;          // ptr to active frame          // todo: ctor default init
        mongo::BSONObj *_activeObj;       // ptr to active JSON object    // todo: ctor default init
        // todo: better pointer to BSONObj, create on the fly? test it.
        mongo::BSONObj _extraOptions;     
        mongo::BSONObj _storageEngineObj;
        mongo::BSONObj _validatorObj;
        mongo::BSONObj _indexOptionDefaultsObj;

        const double _dbVersion;
        const std::string _storageEngine;
    };
}
