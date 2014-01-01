#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"

#include <QMessageBox>
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
        _openDbShellAction = new QAction(this);
        _openDbShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
        VERIFY(connect(_openDbShellAction, SIGNAL(triggered()), SLOT(ui_dbOpenShell())));

        _dbStatsAction = new QAction(this);
        VERIFY(connect(_dbStatsAction, SIGNAL(triggered()), SLOT(ui_dbStatistics())));

        _dbDropAction = new QAction(this);
        VERIFY(connect(_dbDropAction, SIGNAL(triggered()), SLOT(ui_dbDrop())));

        _dbRepairAction = new QAction(this);
        VERIFY(connect(_dbRepairAction, SIGNAL(triggered()), SLOT(ui_dbRepair())));

        _refreshDatabaseAction = new QAction(this);
        VERIFY(connect(_refreshDatabaseAction, SIGNAL(triggered()), SLOT(ui_refreshDatabase())));

        BaseClass::_contextMenu->addAction(_openDbShellAction);
        BaseClass::_contextMenu->addAction(_refreshDatabaseAction);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(_dbStatsAction);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(_dbRepairAction);
        BaseClass::_contextMenu->addAction(_dbDropAction);

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
        _collectionFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        addChild(_collectionFolderItem);

        _javascriptFolderItem = new ExplorerDatabaseCategoryTreeItem(this,Functions);
        _javascriptFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        addChild(_javascriptFolderItem);
        
        _usersFolderItem = new ExplorerDatabaseCategoryTreeItem(this,Users);
        _usersFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        addChild(_usersFolderItem);
        
        retranslateUI();
    }
    
    void ExplorerDatabaseTreeItem::retranslateUI()
    {
        _openDbShellAction->setText(tr("Open Shell"));
        _dbStatsAction->setText(tr("Database Statistics"));
        _dbDropAction->setText(tr("Drop Database.."));
        _dbRepairAction->setText(tr("Repair Database..."));
        _refreshDatabaseAction->setText(tr("Refresh"));
        
        _collectionFolderItem->setText(0, tr("Collections"));
        _javascriptFolderItem->setText(0, tr("Functions"));
        _usersFolderItem->setText(0, tr("Users"));
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
        _collectionFolderItem->setText(0, detail::buildName(tr("Collections"),count));

        QtUtils::clearChildItems(_collectionFolderItem);
        _collectionSystemFolderItem = new ExplorerTreeItem(_collectionFolderItem);
        _collectionSystemFolderItem->setIcon(0, GuiRegistry::instance().folderIcon());
        _collectionSystemFolderItem->setText(0, tr("System"));
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
        _usersFolderItem->setText(0, detail::buildName(tr("Users"),count));

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
        _javascriptFolderItem->setText(0,  detail::buildName(tr("Functions"),count));

        QtUtils::clearChildItems(_javascriptFolderItem);

        for (int i = 0; i < functions.size(); ++i) {
            MongoFunction fun = functions[i];
            addFunctionItem(event->database(), fun);
        }
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseCollectionsLoadingEvent *event)
    {
        _collectionFolderItem->setText(0, detail::buildName(tr("Collections"),-1));
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseFunctionsLoadingEvent *event)
    {
        _javascriptFolderItem->setText(0, detail::buildName(tr("Functions"),-1));
    }

    void ExplorerDatabaseTreeItem::handle(MongoDatabaseUsersLoadingEvent *event)
    {
        _usersFolderItem->setText(0, detail::buildName(tr("Users"),-1));
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
        QString buff = tr("Drop <b>%1</b> database?").arg(QtUtils::toQString(_database->name()));
        int answer = QMessageBox::question(treeWidget(),
            tr("Drop Database"),buff,
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
