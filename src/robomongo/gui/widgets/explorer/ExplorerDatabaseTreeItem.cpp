#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"

#include <QMessageBox>
#include <QAction>
#include <QMenu>

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"
#include "robomongo/gui/GuiRegistry.h"


namespace
{
    void openCurrentDatabaseShell(Robomongo::MongoDatabase *database,const QString &script, bool execute = true, const Robomongo::CursorPosition &cursor = Robomongo::CursorPosition())
    {
        Robomongo::AppRegistry::instance().app()->openShell(database, script, execute, Robomongo::QtUtils::toQString(database->name()), cursor);
    }
}

namespace Robomongo
{
    namespace detail
    {
        QString buildName(const QString& text,int count)
        {
            if (count == -1)
                return QString("%1 ...").arg(text);

            return QString("%1 (%2)").arg(text).arg(count);
        }
    }
    ExplorerDatabaseTreeItem::ExplorerDatabaseTreeItem(QTreeWidgetItem *parent,MongoDatabase *const database) :
        BaseClass(parent),
        _database(database),
        _bus(AppRegistry::instance().bus()),
        _collectionSystemFolderItem(NULL)
    {
        QAction *openDbShellAction = new QAction("Open Shell", this);
        openDbShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
        VERIFY(connect(openDbShellAction, SIGNAL(triggered()), SLOT(ui_dbOpenShell())));

        QAction *dbStats = new QAction("Database Statistics", this);
        VERIFY(connect(dbStats, SIGNAL(triggered()), SLOT(ui_dbStatistics())));

        QAction *dbDrop = new QAction("Drop Database..", this);
        VERIFY(connect(dbDrop, SIGNAL(triggered()), SLOT(ui_dbDrop())));

        QAction *dbRepair = new QAction("Repair Database...", this);
        VERIFY(connect(dbRepair, SIGNAL(triggered()), SLOT(ui_dbRepair())));

        QAction *refreshDatabase = new QAction("Refresh", this);
        VERIFY(connect(refreshDatabase, SIGNAL(triggered()), SLOT(ui_refreshDatabase())));

        BaseClass::_contextMenu->addAction(openDbShellAction);
        BaseClass::_contextMenu->addAction(refreshDatabase);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(dbStats);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(dbRepair);
        BaseClass::_contextMenu->addAction(dbDrop);

        _bus->subscribe(this, MongoDatabaseCollectionListLoadedEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseUsersLoadedEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseFunctionsLoadedEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseCollectionsLoadingEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseFunctionsLoadingEvent::Type, _database);
        _bus->subscribe(this, MongoDatabaseUsersLoadingEvent::Type, _database);
        
        setText(0, QtUtils::toQString(_database->name()));
        setIcon(0, GuiRegistry::instance().databaseIcon());
        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

        _collectionFolderItem = new ExplorerDatabaseCategoryTreeItem(this,Collections);
        _collectionFolderItem->setText(0, "Collections");
        _collectionFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        addChild(_collectionFolderItem);

        _javascriptFolderItem = new ExplorerDatabaseCategoryTreeItem(this,Functions);
        _javascriptFolderItem->setText(0, "Functions");
        _javascriptFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        addChild(_javascriptFolderItem);
        
        _usersFolderItem = new ExplorerDatabaseCategoryTreeItem(this,Users);
        _usersFolderItem->setText(0, "Users");
        _usersFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        addChild(_usersFolderItem);
    }

    void ExplorerDatabaseTreeItem::expandCollections()
    {
        _database->loadCollections();
    }

    void ExplorerDatabaseTreeItem::expandUsers()
    {
        _database->loadUsers();
    }

    void ExplorerDatabaseTreeItem::expandColection(ExplorerCollectionTreeItem *const item)
    {        
         _bus->send(_database->server()->client(), new LoadCollectionIndexesRequest(item, item->collection()->info()));
    }

    void ExplorerDatabaseTreeItem::dropIndexFromCollection(ExplorerCollectionTreeItem *const item, const std::string &indexName)
    {
        _bus->send(_database->server()->client(), new DropCollectionIndexRequest(item, item->collection()->info(), indexName));
    }

    void ExplorerDatabaseTreeItem::enshureIndex(ExplorerCollectionTreeItem *const item, const EnsureIndexInfo &oldInfo, const EnsureIndexInfo &newInfo)
    {
        _bus->send(_database->server()->client(), new EnsureIndexRequest(item,oldInfo ,newInfo));
    }

    void ExplorerDatabaseTreeItem::editIndexFromCollection(ExplorerCollectionTreeItem *const item,const std::string &oldIndexText,const std::string &newIndexText)
    {
         _bus->send(_database->server()->client(), new EditIndexRequest(item, item->collection()->info(),oldIndexText,newIndexText));
    }

    void ExplorerDatabaseTreeItem::expandFunctions()
    {
        _database->loadFunctions();
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseCollectionListLoadedEvent *event)
    {
        std::vector<MongoCollection *> collections = event->collections;
        int count = collections.size();
        _collectionFolderItem->setText(0, detail::buildName("Collections",count));

        QtUtils::clearChildItems(_collectionFolderItem);
        _collectionSystemFolderItem = new ExplorerTreeItem(_collectionFolderItem);
        _collectionSystemFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        _collectionSystemFolderItem->setText(0, "System");
        _collectionFolderItem->addChild(_collectionSystemFolderItem);

        for (int i = 0; i < collections.size(); ++i) {
            MongoCollection *collection = collections[i];

            if (collection->isSystem()) {
                addSystemCollectionItem(collection);
            } else {
                addCollectionItem(collection);
            }
        }

        showCollectionSystemFolderIfNeeded();
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseUsersLoadedEvent *event)
    {
        std::vector<MongoUser> users = event->users();
        int count = users.size();
        _usersFolderItem->setText(0, detail::buildName("Users",count));

        QtUtils::clearChildItems(_usersFolderItem);

        for (int i = 0; i < users.size(); ++i) {
            MongoUser user = users[i];
            addUserItem(event->database(), user);
        }
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseFunctionsLoadedEvent *event)
    {
        std::vector<MongoFunction> functions = event->functions();
        int count = functions.size();
        _javascriptFolderItem->setText(0,  detail::buildName("Functions",count));

        QtUtils::clearChildItems(_javascriptFolderItem);

        for (int i = 0; i < functions.size(); ++i) {
            MongoFunction fun = functions[i];
            addFunctionItem(event->database(), fun);
        }
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseCollectionsLoadingEvent *event)
    {
        _collectionFolderItem->setText(0, detail::buildName("Collections",-1));
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseFunctionsLoadingEvent *event)
    {
        _javascriptFolderItem->setText(0, detail::buildName("Functions",-1));
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseUsersLoadingEvent *event)
    {
        _usersFolderItem->setText(0, detail::buildName("Users",-1));
    }

    void ExplorerDatabaseTreeItem::addCollectionItem(MongoCollection *collection)
    {
        ExplorerCollectionTreeItem *collectionItem = new ExplorerCollectionTreeItem(_collectionFolderItem,this,collection);
        _collectionFolderItem->addChild(collectionItem);
    }

    void ExplorerDatabaseTreeItem::addSystemCollectionItem(MongoCollection *collection)
    {
        ExplorerCollectionTreeItem *collectionItem = new ExplorerCollectionTreeItem(_collectionSystemFolderItem, this, collection);
        _collectionSystemFolderItem->addChild(collectionItem);
    }

    void ExplorerDatabaseTreeItem::showCollectionSystemFolderIfNeeded()
    {
        _collectionSystemFolderItem->setHidden(_collectionSystemFolderItem->childCount() == 0);
    }

    void ExplorerDatabaseTreeItem::addUserItem(MongoDatabase *database, const MongoUser &user)
    {
        ExplorerUserTreeItem *userItem = new ExplorerUserTreeItem(_usersFolderItem, database, user);
        _usersFolderItem->addChild(userItem);
    }

    void ExplorerDatabaseTreeItem::addFunctionItem(MongoDatabase *database, const MongoFunction &function)
    {
        ExplorerFunctionTreeItem *functionItem = new ExplorerFunctionTreeItem(_javascriptFolderItem, database, function);
        _javascriptFolderItem->addChild(functionItem);
    }

    void ExplorerDatabaseTreeItem::ui_refreshDatabase()
    {
        expandCollections();
    }

    void ExplorerDatabaseTreeItem::ui_dbStatistics()
    {
        openCurrentDatabaseShell(_database,"db.stats()");
    }

    void ExplorerDatabaseTreeItem::ui_dbDrop()
    {
        // Ask user
        QString buff = QString("Drop <b>%1</b> database?").arg(QtUtils::toQString(_database->name()));
        int answer = QMessageBox::question(treeWidget(),
            "Drop Database",buff,
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);
        if (answer != QMessageBox::Yes)
            return;

        _database->server()->dropDatabase(_database->name());
        _database->server()->loadDatabases(); // refresh list of databases
    }

    void ExplorerDatabaseTreeItem::ui_dbRepair()
    {
        openCurrentDatabaseShell(_database, "db.repairDatabase()", false);
    }

    void ExplorerDatabaseTreeItem::ui_dbOpenShell()
    {
        openCurrentDatabaseShell(_database, "");
    }
}
