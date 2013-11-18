#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"
#include "robomongo/core/events/MongoEventsInfo.hpp"

namespace Robomongo
{
    namespace detail
    {
        QString buildName(const QString& text,int count);
    }

    class ExplorerCollectionTreeItem;
    class ExplorerDatabaseCategoryTreeItem;
    class MongoDatabase;
    class MongoCollection;

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

        void startCollectionListLoad(const EventsInfo::LoadCollectionRequestInfo &inf);
        void finishCollectionListLoad(const EventsInfo::LoadCollectionResponceInfo &inf);
        
        void startUserListLoad(const EventsInfo::LoadUserRequestInfo &inf);
        void finishUserListLoad(const EventsInfo::LoadUserResponceInfo &inf);

        void startFunctionListLoad(const EventsInfo::LoadFunctionRequestInfo &inf);
        void finishFunctionListLoad(const EventsInfo::LoadFunctionResponceInfo &inf);
    private:
        void addCollectionItem(MongoCollection *collection);
        void addSystemCollectionItem(MongoCollection *collection);
        void showCollectionSystemFolderIfNeeded();

        void addUserItem(MongoDatabase *database, const MongoUser &user);
        void addFunctionItem(MongoDatabase *database, const MongoFunction &function);

        ExplorerDatabaseCategoryTreeItem *_collectionFolderItem;
        ExplorerDatabaseCategoryTreeItem *_javascriptFolderItem;
        ExplorerDatabaseCategoryTreeItem *_usersFolderItem;
        ExplorerTreeItem *_collectionSystemFolderItem;
        MongoDatabase *const _database;
    };
}
