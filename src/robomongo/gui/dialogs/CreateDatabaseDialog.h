#pragma once

#include <QDialog>
#include <QLineEdit>

namespace Robomongo
{
    class CreateDatabaseDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit CreateDatabaseDialog(const QString &serverName, QWidget *parent = 0);
        QString databaseName() const;

    public slots:
        virtual void accept();

    private:
        QLineEdit *_databaseName;
    };
}
