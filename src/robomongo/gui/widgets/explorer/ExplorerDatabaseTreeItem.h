#pragma once
#include <QTreeWidgetItem>
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"

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
        void enshureIndex(ExplorerCollectionTreeItem *const item,const QString& text,bool unique,bool backGround,bool dropDuplicateIndex);

    public slots:
        void handle(MongoDatabaseCollectionListLoadedEvent *event);
        void handle(MongoDatabaseUsersLoadedEvent *event);
        void handle(MongoDatabaseFunctionsLoadedEvent *event);
        void handle(MongoDatabaseCollectionsLoadingEvent *event);
        void handle(MongoDatabaseFunctionsLoadingEvent *event);
        void handle(MongoDatabaseUsersLoadingEvent *event);
        
    private:
        void clearChildItems(QTreeWidgetItem *root);
        void createCollectionSystemFolderItem();
        void addCollectionItem(MongoCollection *collection);
        void addSystemCollectionItem(MongoCollection *collection);
        void showCollectionSystemFolderIfNeeded();

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
