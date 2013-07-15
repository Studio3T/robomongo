#pragma once
#include <QTreeWidgetItem>
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"

namespace Robomongo
{
    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseCategoryTreeItem;
    class EventBus;
    class MongoDatabase_CollectionListLoadedEvent;
    class MongoDatabase_UsersLoadedEvent;
    class MongoDatabase_FunctionsLoadedEvent;
    class MongoDatabase_CollectionsLoadingEvent;
    class MongoDatabase_FunctionsLoadingEvent;
    class MongoDatabase_UsersLoadingEvent;

    class ExplorerDatabaseTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    public:
        ExplorerDatabaseTreeItem(MongoDatabase *database);

        MongoDatabase *database() const { return _database; }
        void expandCollections();
        void expandUsers();
        void expandFunctions();
        void expandColection(ExplorerCollectionTreeItem *const item);
        void deleteIndexFromCollection(ExplorerCollectionTreeItem *const item,const QString& indexText); 

    public slots:
        void handle(MongoDatabase_CollectionListLoadedEvent *event);
        void handle(MongoDatabase_UsersLoadedEvent *event);
        void handle(MongoDatabase_FunctionsLoadedEvent *event);
        void handle(MongoDatabase_CollectionsLoadingEvent *event);
        void handle(MongoDatabase_FunctionsLoadingEvent *event);
        void handle(MongoDatabase_UsersLoadingEvent *event);
        
    private:
        void clearChildItems(QTreeWidgetItem *root);
        void createCollectionSystemFolderItem();
        void addCollectionItem(MongoCollection *collection);
        void addSystemCollectionItem(MongoCollection *collection);
        void showCollectionSystemFolderIfNeeded();

        /**
         * @brief Builds folder names for corresponding categories.
         * @param count: Number of items.
         *               If NULL - name will be without count of items.
         *               If -1   - name will contain "..." at the end.
         */
        QString buildCollectionsFolderName(int *count = NULL);
        QString buildUsersFolderName(int *count = NULL);
        QString buildFunctionsFolderName(int *count = NULL);

        void addUserItem(MongoDatabase *database, const MongoUser &user);
        void addFunctionItem(MongoDatabase *database, const MongoFunction &function);

        EventBus *_bus;
        ExplorerDatabaseCategoryTreeItem *_collectionFolderItem;
        ExplorerDatabaseCategoryTreeItem *_javascriptFolderItem;
        ExplorerDatabaseCategoryTreeItem *_usersFolderItem;
        ExplorerDatabaseCategoryTreeItem *_filesFolderItem;
        QTreeWidgetItem *_collectionSystemFolderItem;
        MongoDatabase *_database;
    };
}
