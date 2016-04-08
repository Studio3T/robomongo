#pragma once

#include <QDialog>
#include <mongo/bson/bsonobj.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QDialogButtonBox;
class QLineEdit;
class QTabWidget;
class QCheckBox;
QT_END_NAMESPACE
class QsciScintilla;

namespace Robomongo
{
    class FindFrame;
    class Indicator;

    class CreateCollectionDialog : public QDialog
    {
        Q_OBJECT

    public:
        typedef std::vector<mongo::BSONObj> ReturnType;     // todo: public access?

        explicit CreateCollectionDialog(const QString &serverName,
            const QString &database = QString(),
            const QString &collection = QString(), QWidget *parent = 0);

        QString jsonText() const;
        /**
        * @brief Use returned BSONObj only if Dialog exec() method returns QDialog::Accepted
        */
        ReturnType bsonObj() const { return _obj; }
        void setCursorPosition(int line, int column);

        QString getCollectionName() const;
        void setOkButtonText(const QString &text);
        void setInputLabelText(const QString &text);
        void setInputText(const QString &text);

        // Options Tab
        bool isCapped() const;
        long long getSizeInputEditValue() const;
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
        bool validate(bool silentOnSuccess = true);

        private Q_SLOTS:
        void onFrameTextChanged();
        void onValidateButtonClicked();
        void enableFindButton(const QString &text);

    private:
        Indicator *createDatabaseIndicator(const QString &database);
        Indicator *createCollectionIndicator(const QString &collection);
        QWidget *createOptionsTab();
        QWidget *createStorageEngineTab();
        void configureQueryText();

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
        FindFrame *_frameText;
        QLabel * _frameLabel;
        // Validator Tab
        //...

        ReturnType _obj;
    };
}
