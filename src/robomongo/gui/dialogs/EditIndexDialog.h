#pragma once
#include <QDialog>
QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE
namespace Robomongo
{
    class EditItemIndexDialog: public QDialog
    {
        Q_OBJECT
    public:
        typedef QDialog BaseClass;
        enum
        {
            HeightWidget = 80,
            WidthWidget = 240
        };
        explicit EditItemIndexDialog(QWidget *parent,const QString &text,const QString &collectionName,const QString &databaseName);
        QString text()const;
    public Q_SLOTS:
        virtual void accept();
    private:
       QLineEdit *_indexName;
    };
}
