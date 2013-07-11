#pragma once

#include <QObject>
#include <QTreeWidgetItem>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoFunction.h"

namespace Robomongo
{
    class ExplorerFunctionTreeItem : public QTreeWidgetItem
    {
    public:
        ExplorerFunctionTreeItem(MongoDatabase *database, const MongoFunction &function);
        MongoFunction function() const { return _function; }
        MongoDatabase *database() const { return _database; }

    private:
        QString buildToolTip(const MongoFunction &function);
        MongoFunction _function;
        MongoDatabase *_database;
    };
}

