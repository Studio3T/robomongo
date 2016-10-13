#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    namespace detail
    {
        QString buildName(const QString& text, int count);
    }

    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseCategoryTreeItem;
    class EventBus;
    class MongoDatabaseCollectionListLoadedEvent;
    class MongoDatabaseUsersLoadedEvent;
    class MongoDatabaseFunctionsLoadedEvent;
    class MongoDatabaseCollectionsLoadingEvent;
    class MongoDatabaseFunctionsLoadingEvent;
    class MongoDatabaseUsersLoadingEvent;
    class MongoDatabase;
    class MongoUser;
    class MongoFunction;
    class MongoCollection;
    struct EnsureIndexInfo;

    class ExplorerDatabaseTreeItem : public ExplorerTreeItem
    {
        Q_OBJECT

    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerDatabaseTreeItem(QTreeWidgetItem *parent, MongoDatabase *const database);

        MongoDatabase *database() const { return _database; }
        void expandCollections();
        void expandUsers();
        void expandFunctions();
        void expandColection(ExplorerCollectionTreeItem *const item);
        void dropIndexFromCollection(ExplorerCollectionTreeItem *const item, const std::string &indexName);
        void enshureIndex(ExplorerCollectionTreeItem *const item, const EnsureIndexInfo &oldInfo, const EnsureIndexInfo &newInfo);
        void editIndexFromCollection(ExplorerCollectionTreeItem *const item, const std::string& oldIndexText, const std::string& newIndexText);

    public Q_SLOTS:
        void handle(MongoDatabaseCollectionListLoadedEvent *event);
        void handle(MongoDatabaseUsersLoadedEvent *event);
        void handle(MongoDatabaseFunctionsLoadedEvent *event);
        void handle(MongoDatabaseCollectionsLoadingEvent *event);
        void handle(MongoDatabaseFunctionsLoadingEvent *event);
        void handle(MongoDatabaseUsersLoadingEvent *event);

    private Q_SLOTS:
        void ui_dbStatistics();
        void ui_dbCurrentOps();
        void ui_dbKillOp();
        void ui_dbDrop();
        void ui_dbRepair();
        void ui_dbOpenShell();
        void ui_refreshDatabase();

    private:
        void addCollectionItem(MongoCollection *collection);
        void addSystemCollectionItem(MongoCollection *collection);
        void showCollectionSystemFolderIfNeeded();

        void addUserItem(MongoDatabase *database, const MongoUser &user);
        void addFunctionItem(MongoDatabase *database, const MongoFunction &function);

        EventBus *_bus;
        ExplorerDatabaseCategoryTreeItem *_collectionFolderItem;
        ExplorerDatabaseCategoryTreeItem *_functionsFolderItem;
        ExplorerDatabaseCategoryTreeItem *_usersFolderItem;
        ExplorerTreeItem *_collectionSystemFolderItem;
        MongoDatabase *const _database;
    };
}
