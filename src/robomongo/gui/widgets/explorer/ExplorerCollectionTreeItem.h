#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"
#include "robomongo/core/domain/CursorPosition.h"
#include "robomongo/core/events/MongoEventsInfo.hpp"
#include "robomongo/core/domain/MongoCollection.h"

namespace Robomongo
{
    class LoadCollectionIndexesResponse;
    class DropCollectionIndexResponse;
    class ExplorerCollectionDirIndexesTreeItem;
    class ExplorerDatabaseTreeItem;
    class MongoServer;

    class ExplorerCollectionTreeItem: public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerCollectionTreeItem(MongoServer *server, QTreeWidgetItem *parent, ExplorerDatabaseTreeItem *databaseItem, MongoCollection *collection);
        MongoCollection *collection() const { return _collection; }
        void expand();
        void openCurrentCollectionShell(const QString &script, bool execute = true, const CursorPosition &cursor = CursorPosition());
        ExplorerDatabaseTreeItem *const databaseItem() const { return _databaseItem; }
        
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

        void startIndexListLoad(const EventsInfo::LoadCollectionIndexesRequestInfo &inf);
        void finishIndexListLoad(const EventsInfo::LoadCollectionIndexesResponceInfo &inf);

    private:
        QString buildToolTip(MongoCollection *collection);
        ExplorerCollectionDirIndexesTreeItem *_indexDir;
        MongoCollection *const _collection;
        ExplorerDatabaseTreeItem *const _databaseItem;
        const MongoServer *const _server;
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
