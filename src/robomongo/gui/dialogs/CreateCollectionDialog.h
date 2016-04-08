#pragma once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QLabel;
class QDialogButtonBox;
class QLineEdit;
class QTabWidget;
class QCheckBox;
QT_END_NAMESPACE

namespace Robomongo
{
    class Indicator;

    class CreateCollectionDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit CreateCollectionDialog(const QString &serverName,
            const QString &database = QString(),
            const QString &collection = QString(), QWidget *parent = 0);
        QString getCollectionName() const;
        void setOkButtonText(const QString &text);
        void setInputLabelText(const QString &text);
        void setInputText(const QString &text);
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

    private:
        Indicator *createDatabaseIndicator(const QString &database);
        Indicator *createCollectionIndicator(const QString &collection);
        QWidget *createOptionsTab();
        QWidget *createStorageEngineTab();

        QLineEdit *_inputEdit;
        QLabel *_inputLabel;
        QTabWidget *_tabWidget;
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
        // ... todo

        QDialogButtonBox *_buttonBox;
    };
}
