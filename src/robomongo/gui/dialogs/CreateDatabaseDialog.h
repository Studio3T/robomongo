#pragma once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QLabel;
class QDialogButtonBox;
class QLineEdit;
QT_END_NAMESPACE

namespace Robomongo
{
    class Indicator;

    class CreateDatabaseDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit CreateDatabaseDialog(const QString &serverName,
                                      const QString &database = QString(),
                                      const QString &collection = QString(), QWidget *parent = 0);
        QString databaseName() const;
        void setOkButtonText(const QString &text);
        void setInputLabelText(const QString &text);
        void setInputText(const QString &text);


    public slots:
        virtual void accept();

    private:
        Indicator *createDatabaseIndicator(const QString &database);
        Indicator *createCollectionIndicator(const QString &collection);
        QLineEdit *_inputEdit;
        QLabel *_inputLabel;
        QDialogButtonBox *_buttonBox;
    };
}
