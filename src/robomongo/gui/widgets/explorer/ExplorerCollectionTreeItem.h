#pragma once
#include <QTreeWidgetItem>
#include <QObject>
#include "robomongo/core/Event.h"

namespace Robomongo
{
    class ExplorerDatabaseTreeItem;
    class LoadCollectionIndexesResponse;
    class DeleteCollectionIndexResponse;
    class ExplorerCollectionDirIndexesTreeItem;
    class CollectionIndexesLoadingEvent : public Event
    {
        R_EVENT
            CollectionIndexesLoadingEvent(QObject *sender) : Event(sender) {}
    };


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
        void handle(CollectionIndexesLoadingEvent *event);
    private:
        QString buildToolTip(MongoCollection *collection);
        ExplorerCollectionDirIndexesTreeItem * const _indexDir;
        MongoCollection *const _collection;
    };

    class ExplorerCollectionDirIndexesTreeItem: public QTreeWidgetItem
    {
    public:
        static const QString text;
        explicit ExplorerCollectionDirIndexesTreeItem(ExplorerCollectionTreeItem *const parent);
    };

    class ExplorerCollectionIndexesTreeItem: public QTreeWidgetItem
    {
    public:
        explicit ExplorerCollectionIndexesTreeItem(const QString &val,ExplorerCollectionDirIndexesTreeItem *const parent);
    };
}
