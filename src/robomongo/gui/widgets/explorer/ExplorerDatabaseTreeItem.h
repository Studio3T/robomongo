#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    namespace detail
    {
        QString buildName(const QString& text,int count);
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
    struct EnsureIndex;

    class ExplorerDatabaseTreeItem : public ExplorerTreeItem
    {
        Q_OBJECT

    public:
        typedef ExplorerTreeItem BaseClass;
        ExplorerDatabaseTreeItem(QTreeWidgetItem *parent,MongoDatabase *const database);

        MongoDatabase *database() const { return _database; }
        void expandCollections();
        void expandUsers();
        void expandFunctions();

    private Q_SLOTS:
        void ui_dbStatistics();
        void ui_dbDrop();
        void ui_dbRepair();
        void ui_dbOpenShell();
        void ui_refreshDatabase();

        void collectionListLoad(const std::vector<MongoCollection *>&);
        void startCollectionListLoad();
        void startUsersLoad();
        void userListLoad(const std::vector<MongoUser>&);
        void startFunctionsLoad();
        void functionsListLoad(const std::vector<MongoFunction>&);
    private:
        void addCollectionItem(MongoCollection *collection);
        void addSystemCollectionItem(MongoCollection *collection);
        void showCollectionSystemFolderIfNeeded();

        void addUserItem(MongoDatabase *database, const MongoUser &user);
        void addFunctionItem(MongoDatabase *database, const MongoFunction &function);

        EventBus *_bus;
        ExplorerDatabaseCategoryTreeItem *_collectionFolderItem;
        ExplorerDatabaseCategoryTreeItem *_javascriptFolderItem;
        ExplorerDatabaseCategoryTreeItem *_usersFolderItem;
        ExplorerTreeItem *_collectionSystemFolderItem;
        MongoDatabase *const _database;
    };
}
