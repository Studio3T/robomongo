#pragma once

#include "robomongo/core/Event.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"
#include "robomongo/core/domain/CursorPosition.h"
#include "robomongo/core/events/MongoEventsInfo.h"
#include "robomongo/core/domain/MongoCollection.h"

namespace Robomongo
{
    class LoadCollectionIndexesResponse;
    class DropCollectionIndexResponse;
    class ExplorerCollectionDirIndexesTreeItem;
    class ExplorerDatabaseTreeItem;

    class CollectionIndexesLoadingEvent : public Event
    {
        R_EVENT
            CollectionIndexesLoadingEvent(QObject *sender) : Event(sender) {}
    };

    class ExplorerCollectionTreeItem: public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerCollectionTreeItem(QObject *reciver, QTreeWidgetItem *parent, ExplorerDatabaseTreeItem *databaseItem, MongoCollection *collection);
        MongoCollection *collection() const { return _collection; }
        void expand();
        void openCurrentCollectionShell(const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());
        ExplorerDatabaseTreeItem *const databaseItem() const { return _databaseItem; }
        virtual void customEvent(QEvent *); 
        void createIndex(const EnsureIndex &oldInfo, const EnsureIndex &newInfo);
        void deleteIndex(const std::string &indexName);

    public Q_SLOTS:
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
        void ui_copyToCollectionToDiffrentServer();
        void ui_viewCollection();

    private:
        void loadIndexes();
        QString buildToolTip(MongoCollection *collection);
        ExplorerCollectionDirIndexesTreeItem *_indexDir;
        MongoCollection *const _collection;
        ExplorerDatabaseTreeItem *const _databaseItem;
        QObject *_reciver;
    };

    class ExplorerCollectionDirIndexesTreeItem: public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        static const QString labelText;
        explicit ExplorerCollectionDirIndexesTreeItem(QTreeWidgetItem *parent);
        void expand();

    private Q_SLOTS:
        void ui_addIndex();
        void ui_addIndexGui();
        void ui_reIndex();
        void ui_dropIndex();
        void ui_viewIndex();
        void ui_refreshIndex();
    };

    class ExplorerCollectionIndexesTreeItem: public ExplorerTreeItem
    {
         Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        explicit ExplorerCollectionIndexesTreeItem(ExplorerCollectionDirIndexesTreeItem *parent,const EnsureIndex &info);

    private Q_SLOTS:
        void ui_dropIndex();
        void ui_edit();
    private:
        EnsureIndex _info;
    };
}
