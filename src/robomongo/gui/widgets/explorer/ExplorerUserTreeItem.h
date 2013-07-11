#pragma once

#include <QObject>
#include <QTreeWidgetItem>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoUser.h"

namespace Robomongo
{
    class ExplorerUserTreeItem : public QTreeWidgetItem
    {
    public:
        ExplorerUserTreeItem(MongoDatabase *database, const MongoUser &user);
        MongoUser user() const { return _user; }
        MongoDatabase *database() const { return _database; }

    private:
        QString buildToolTip(const MongoUser &user);
        MongoUser _user;
        MongoDatabase *_database;
    };
}

