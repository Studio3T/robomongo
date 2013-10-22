#pragma once

#include <QDialog>
#include "robomongo/core/domain/App.h"
QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QComboBox;
QT_END_NAMESPACE

namespace Robomongo
{
    class MongoDatabase;

    class CopyCollection : public QDialog
    {
        Q_OBJECT

    public:
        static const QSize minimumSize;

        explicit CopyCollection(const QString &serverName,
                                      const QString &database,
                                      const QString &collection, QWidget *parent = 0);

    public Q_SLOTS:
        virtual void accept();
        void updateDatabaseComboBox(int index);
        MongoDatabase *selectedDatabase();
    private:
        App::MongoServersContainerType _servers;
        const QString _currentServerName;
        const QString _currentDatabase;
        QComboBox *_serverComboBox;
        QComboBox *_databaseComboBox;
        QDialogButtonBox *_buttonBox;
    };
}
