#pragma once

#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    class MongoDatabase;

    class ExplorerFunctionTreeItem :public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerFunctionTreeItem(QTreeWidgetItem *parent, MongoDatabase *database, const MongoFunction &function);
        MongoFunction function() const { return _function; }
        MongoDatabase *database() const { return _database; }

    private Q_SLOTS:
        void ui_editFunction();        
        void ui_dropFunction();

    private:
        QString buildToolTip(const MongoFunction &function);
        MongoFunction _function;
        MongoDatabase *_database;
    };
}

