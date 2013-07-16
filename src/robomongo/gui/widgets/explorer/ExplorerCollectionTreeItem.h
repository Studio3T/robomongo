#pragma once
#include <QTreeWidgetItem>
#include <QObject>
#include "robomongo/core/Event.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"
#include "robomongo/core/domain/CursorPosition.h"

namespace Robomongo
{
    class LoadCollectionIndexesResponse;
    class DeleteCollectionIndexResponse;
    class ExplorerCollectionDirIndexesTreeItem;
    class ExplorerDatabaseTreeItem;
    class CollectionIndexesLoadingEvent : public Event
    {
        R_EVENT
            CollectionIndexesLoadingEvent(QObject *sender) : Event(sender) {}
    };


    class ExplorerCollectionTreeItem :public QObject, public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerCollectionTreeItem(QTreeWidgetItem *parent,ExplorerDatabaseTreeItem *databaseItem,MongoCollection *collection);
        MongoCollection *collection() const { return _collection; }
        void expand();
        void deleteIndex(const QTreeWidgetItem * const ind);
    public Q_SLOTS:
        void handle(LoadCollectionIndexesResponse *event);
        void handle(DeleteCollectionIndexResponse *event);
        void handle(CollectionIndexesLoadingEvent *event);
    private Q_SLOTS:
        void ui_addDocument();
        void ui_removeDocument();
        void ui_updateDocument();
        void ui_collectionStatistics();
        void ui_removeAllDocuments();
        void ui_storageSize();
        void ui_totalIndexSize();
        void ui_totalSize();
        void ui_shardVersion();
        void ui_shardDistribution();
        void ui_dropCollection();
        void ui_renameCollection();
        void ui_duplicateCollection();
        void ui_viewCollection();

    private:
        void openCurrentCollectionShell(const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());
        QString buildToolTip(MongoCollection *collection);
        ExplorerCollectionDirIndexesTreeItem * const _indexDir;
        MongoCollection *const _collection;
        ExplorerDatabaseTreeItem *const _databaseItem;
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
