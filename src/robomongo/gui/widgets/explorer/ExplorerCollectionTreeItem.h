#pragma once
#include <QTreeWidgetItem>
#include <QObject>
#include "robomongo/core/Core.h"

namespace Robomongo
{
    class ExplorerDatabaseTreeItem;
    class LoadCollectionIndexesResponse;
    class DeleteCollectionIndexResponse;
    
    class ExplorerCollectionTreeItem :public QObject, public QTreeWidgetItem
    {
        Q_OBJECT
    public:
        ExplorerCollectionTreeItem(ExplorerDatabaseTreeItem *const parent,MongoCollection *collection);
        MongoCollection *collection() const { return _collection; }
        void expand();
        void deleteIndex(const QTreeWidgetItem * const ind);
    public Q_SLOTS:
        void handle(LoadCollectionIndexesResponse *event);
        void handle(DeleteCollectionIndexResponse *event);
    private:
        QString buildToolTip(MongoCollection *collection);
        MongoCollection *const _collection;
    };

    class Indexes: public QTreeWidgetItem
    {
    public:
        explicit Indexes(const QString &val,ExplorerCollectionTreeItem *const parent);
    };
}
