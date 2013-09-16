#pragma once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QComboBox;
QT_END_NAMESPACE

namespace Robomongo
{
    class Indicator;

    class CopyCollection : public QDialog
    {
        Q_OBJECT

    public:
        explicit CopyCollection(const QString &serverName,
                                      const QString &database,
                                      const QString &collection, QWidget *parent = 0);

    public Q_SLOTS:
        virtual void accept();

    private:
        QComboBox *_server;
        QComboBox *_database;
        QDialogButtonBox *_buttonBox;
    };
}
