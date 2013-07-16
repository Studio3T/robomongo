#pragma once

#include <QObject>
#include <QTreeWidgetItem>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    class ExplorerUserTreeItem : public QObject, public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerUserTreeItem(QTreeWidgetItem *parent,MongoDatabase *database, const MongoUser &user);
        MongoUser user() const { return _user; }
        MongoDatabase *database() const { return _database; }

    private Q_SLOTS:
        void ui_dropUser();
        void ui_editUser();
    private:
        QString buildToolTip(const MongoUser &user);
        MongoUser _user;
        MongoDatabase *_database;
    };
}

