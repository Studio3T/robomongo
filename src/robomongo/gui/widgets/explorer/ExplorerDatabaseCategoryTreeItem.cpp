#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/gui/dialogs/FunctionTextEditor.h"
#include "robomongo/gui/dialogs/CreateUserDialog.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/dialogs/CreateCollectionDialog.h"
#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    void openDatabaseShell(Robomongo::MongoDatabase *database, const QString &script, bool execute = true, const Robomongo::CursorPosition &cursor = Robomongo::CursorPosition())
    {
        Robomongo::AppRegistry::instance().app()->openShell(database, script, execute, Robomongo::QtUtils::toQString(database->name()), cursor);
    }
}

namespace Robomongo
{

    ExplorerDatabaseCategoryTreeItem::ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseTreeItem *databaseItem, ExplorerDatabaseCategory category) :
        BaseClass(databaseItem), _category(category)
    {
        if (_category == Collections) {
            QAction *createCollection = new QAction("Create Collection...", this);
            VERIFY(connect(createCollection, SIGNAL(triggered()), SLOT(ui_createCollection())));

            QAction *dbCollectionsStats = new QAction("Collections Statistics", this);
            VERIFY(connect(dbCollectionsStats, SIGNAL(triggered()), SLOT(ui_dbCollectionsStatistics())));

            QAction *refreshCollections = new QAction("Refresh", this);
            VERIFY(connect(refreshCollections, SIGNAL(triggered()), SLOT(ui_refreshCollections())));

            BaseClass::_contextMenu->addAction(dbCollectionsStats);
            BaseClass::_contextMenu->addAction(createCollection);
            BaseClass::_contextMenu->addSeparator();
            BaseClass::_contextMenu->addAction(refreshCollections);
        }
        else if (_category == Users) {

            QAction *refreshUsers = new QAction("Refresh", this);
            VERIFY(connect(refreshUsers, SIGNAL(triggered()), SLOT(ui_refreshUsers())));

            QAction *viewUsers = new QAction("View Users", this);
            VERIFY(connect(viewUsers, SIGNAL(triggered()), SLOT(ui_viewUsers())));

            QAction *addUser = new QAction("Add User...", this);
            VERIFY(connect(addUser, SIGNAL(triggered()), SLOT(ui_addUser())));

            BaseClass::_contextMenu->addAction(viewUsers);
            BaseClass::_contextMenu->addAction(addUser);
            BaseClass::_contextMenu->addSeparator();
            BaseClass::_contextMenu->addAction(refreshUsers);
        }
        else if (_category == Functions) {

            QAction *refreshFunctions = new QAction("Refresh", this);
            VERIFY(connect(refreshFunctions, SIGNAL(triggered()), SLOT(ui_refreshFunctions())));

            QAction *viewFunctions = new QAction("View Functions", this);
            VERIFY(connect(viewFunctions, SIGNAL(triggered()), SLOT(ui_viewFunctions())));

            QAction *addFunction = new QAction("Add Function...", this);
            VERIFY(connect(addFunction, SIGNAL(triggered()), SLOT(ui_addFunction())));

            BaseClass::_contextMenu->addAction(viewFunctions);
            BaseClass::_contextMenu->addAction(addFunction);
            BaseClass::_contextMenu->addSeparator();
            BaseClass::_contextMenu->addAction(refreshFunctions);
        }

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
    
    void ExplorerDatabaseCategoryTreeItem::expand()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (!databaseItem)
            return;

        switch(_category) {
        case Collections:
            databaseItem->expandCollections();
            break;
        case Files:
             break;
        case Functions:
            databaseItem->expandFunctions();
            break;
        case Users:
            databaseItem->expandUsers();
            break;
        }
    }

    ExplorerDatabaseTreeItem *ExplorerDatabaseCategoryTreeItem::databaseItem() const 
    { 
        return static_cast<ExplorerDatabaseTreeItem*>(parent()); 
    }

    void ExplorerDatabaseCategoryTreeItem::ui_dbCollectionsStatistics()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (databaseItem) {
            openDatabaseShell(databaseItem->database(), "db.printCollectionStats()");
        }
    } 

    void ExplorerDatabaseCategoryTreeItem::ui_refreshUsers()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (databaseItem) {
            databaseItem->expandUsers();
        }
    }

    void ExplorerDatabaseCategoryTreeItem::ui_refreshFunctions()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (databaseItem) {
            databaseItem->expandFunctions();
        }
    }

    void ExplorerDatabaseCategoryTreeItem::ui_viewUsers()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (databaseItem) {
            openDatabaseShell(databaseItem->database(), "db.system.users.find()");
        }
    }

    void ExplorerDatabaseCategoryTreeItem::ui_viewFunctions()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (databaseItem) {
            openDatabaseShell(databaseItem->database(), "db.system.js.find()");
        }
    }

    void ExplorerDatabaseCategoryTreeItem::ui_createCollection()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (!databaseItem)
            return;

        const float dbVersion = databaseItem->database()->server()->version();
        const std::string& engineName = databaseItem->database()->server()->getStorageEngineType();
        const QString& serverName = QtUtils::toQString(databaseItem->database()->server()->connectionRecord()->getFullAddress());
        const QString& dbName = QtUtils::toQString(databaseItem->database()->name());

        CreateCollectionDialog dlg(serverName, dbVersion, engineName, dbName, QString(), treeWidget());
        int result = dlg.exec();
        if (result != QDialog::Accepted)
            return;
        std::string collectionName = QtUtils::toStdString(dlg.getCollectionName());
        databaseItem->database()->createCollection(collectionName, 
            dlg.getSizeInputValue(), dlg.isCapped(), dlg.getMaxDocNumberInputValue(), dlg.getExtraOptions());
    }

    void ExplorerDatabaseCategoryTreeItem::ui_addUser()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (!databaseItem)
            return;

        std::unique_ptr<CreateUserDialog> dlg = nullptr;

        float const version = databaseItem->database()->server()->version();
        if (version < MongoUser::minimumSupportedVersion) {
            dlg.reset(new CreateUserDialog(
                QtUtils::toQString(databaseItem->database()->server()->connectionRecord()->getFullAddress()),
                QtUtils::toQString(databaseItem->database()->name()), MongoUser(version), treeWidget()));
        }
        else {
            dlg.reset(new CreateUserDialog(databaseItem->database()->server()->getDatabasesNames(), 
                QtUtils::toQString(databaseItem->database()->server()->connectionRecord()->getFullAddress()),
                QtUtils::toQString(databaseItem->database()->name()), MongoUser(version), treeWidget()));
        }

        if (dlg->exec() != QDialog::Accepted)
            return;

        MongoUser user = dlg->user();
        databaseItem->database()->createUser(user, false);
    }

    void ExplorerDatabaseCategoryTreeItem::ui_addFunction()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (!databaseItem)
            return;

        FunctionTextEditor dlg(
            QtUtils::toQString(databaseItem->database()->server()->connectionRecord()->getFullAddress()), 
            QtUtils::toQString(databaseItem->database()->name()), MongoFunction());

        dlg.setWindowTitle("Create Function");
        dlg.setCode(
            "function() {\n"
            "    // write your code here\n"
            "}");
        
        if (dlg.exec() != QDialog::Accepted)
            return;

        MongoFunction function = dlg.function();
        databaseItem->database()->createFunction(function);
    }

    void ExplorerDatabaseCategoryTreeItem::ui_refreshCollections()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (databaseItem) 
            databaseItem->expandCollections();
    }
}
