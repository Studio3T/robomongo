#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/gui/dialogs/FunctionTextEditor.h"
#include "robomongo/gui/dialogs/CreateUserDialog.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
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

    ExplorerDatabaseCategoryTreeItem::ExplorerDatabaseCategoryTreeItem(ExplorerDatabaseTreeItem *databaseItem,ExplorerDatabaseCategory category) :
        BaseClass(databaseItem) ,_category(category)
    {
        if (_category == Collections) {
            _createCollectionAction = new QAction(this);
            VERIFY(connect(_createCollectionAction, SIGNAL(triggered()), SLOT(ui_createCollection())));

            _dbCollectionsStatsAction = new QAction(this);
            VERIFY(connect(_dbCollectionsStatsAction, SIGNAL(triggered()), SLOT(ui_dbCollectionsStatistics())));

            _refreshCollectionsAction = new QAction(this);
            VERIFY(connect(_refreshCollectionsAction, SIGNAL(triggered()), SLOT(ui_refreshCollections())));

            BaseClass::_contextMenu->addAction(_dbCollectionsStatsAction);
            BaseClass::_contextMenu->addAction(_createCollectionAction);
            BaseClass::_contextMenu->addSeparator();
            BaseClass::_contextMenu->addAction(_refreshCollectionsAction);
        }
        else if (_category == Users) {

            _refreshUsersAction = new QAction(this);
            VERIFY(connect(_refreshUsersAction, SIGNAL(triggered()), SLOT(ui_refreshUsers())));

            _viewUsersAction = new QAction(this);
            VERIFY(connect(_viewUsersAction, SIGNAL(triggered()), SLOT(ui_viewUsers())));

            _addUserAction = new QAction(this);
            VERIFY(connect(_addUserAction, SIGNAL(triggered()), SLOT(ui_addUser())));

            BaseClass::_contextMenu->addAction(_viewUsersAction);
            BaseClass::_contextMenu->addAction(_addUserAction);
            BaseClass::_contextMenu->addSeparator();
            BaseClass::_contextMenu->addAction(_refreshUsersAction);
        }
        else if (_category == Functions) {

            _refreshFunctionsAction = new QAction(this);
            VERIFY(connect(_refreshFunctionsAction, SIGNAL(triggered()), SLOT(ui_refreshFunctions())));

            _viewFunctionsAction = new QAction(this);
            VERIFY(connect(_viewFunctionsAction, SIGNAL(triggered()), SLOT(ui_viewFunctions())));

            _addFunctionAction = new QAction(this);
            VERIFY(connect(_addFunctionAction, SIGNAL(triggered()), SLOT(ui_addFunction())));

            BaseClass::_contextMenu->addAction(_viewFunctionsAction);
            BaseClass::_contextMenu->addAction(_addFunctionAction);
            BaseClass::_contextMenu->addSeparator();
            BaseClass::_contextMenu->addAction(_refreshFunctionsAction);
        }

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        
        retranslateUI();
    }

    void ExplorerDatabaseCategoryTreeItem::retranslateUI()
    {
        if (_category == Collections) {
            _createCollectionAction->setText(tr("Create Collection..."));
            _dbCollectionsStatsAction->setText(tr("Collections Statistics"));
            _refreshCollectionsAction->setText(tr("Refresh"));
        }
        else if (_category == Users) {
            _refreshUsersAction->setText(tr("Refresh"));
            _viewUsersAction->setText(tr("View Users"));
            _addUserAction->setText(tr("Add User..."));
        }
        else if (_category == Functions) {
            _refreshFunctionsAction->setText(tr("Refresh"));
            _viewFunctionsAction->setText(tr("View Functions"));
            _addFunctionAction->setText(tr("Add Function..."));
        }
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

        CreateDatabaseDialog dlg(QtUtils::toQString(databaseItem->database()->server()->connectionRecord()->getFullAddress()),
            QtUtils::toQString(databaseItem->database()->name()), QString(), treeWidget());
        dlg.setWindowTitle(tr("Create Collection"));
        dlg.setOkButtonText(tr("&Create"));
        dlg.setInputLabelText(tr("Collection Name:"));
        int result = dlg.exec();
        if (result != QDialog::Accepted)
            return;

        databaseItem->database()->createCollection(QtUtils::toStdString(dlg.databaseName()));
        // refresh list of databases
        databaseItem->expandCollections();
    }

    void ExplorerDatabaseCategoryTreeItem::ui_addUser()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (!databaseItem)
            return;

        float version = databaseItem->database()->server()->version();
        CreateUserDialog *dlg = NULL;

        if (version < MongoUser::minimumSupportedVersion) {
            dlg = new CreateUserDialog(QtUtils::toQString(databaseItem->database()->server()->connectionRecord()->getFullAddress()),
                QtUtils::toQString(databaseItem->database()->name()), MongoUser(version), treeWidget());
        }
        else {
            dlg = new CreateUserDialog(databaseItem->database()->server()->getDatabasesNames(), QtUtils::toQString(databaseItem->database()->server()->connectionRecord()->getFullAddress()),
            QtUtils::toQString(databaseItem->database()->name()), MongoUser(version), treeWidget());
        }

        int result = dlg->exec();
        if (result != QDialog::Accepted)
            return;

        MongoUser user = dlg->user();
        databaseItem->database()->createUser(user, false);
        // refresh list of users
        databaseItem->expandUsers();
    }

    void ExplorerDatabaseCategoryTreeItem::ui_addFunction()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (!databaseItem)
            return;

        FunctionTextEditor dlg(QtUtils::toQString(databaseItem->database()->server()->connectionRecord()->getFullAddress()), QtUtils::toQString(databaseItem->database()->name()), MongoFunction());
        dlg.setWindowTitle(tr("Create Function"));
        dlg.setCode(
            "function() {\n"
            "    // " + tr("write your code here") + "\n"
            "}");
        dlg.setCursorPosition(1, 4);
        int result = dlg.exec();
        if (result != QDialog::Accepted)
            return;

        MongoFunction function = dlg.function();
        databaseItem->database()->createFunction(function);
        // refresh list of functions
        databaseItem->expandFunctions();
    }

    void ExplorerDatabaseCategoryTreeItem::ui_refreshCollections()
    {
        ExplorerDatabaseTreeItem *databaseItem = ExplorerDatabaseCategoryTreeItem::databaseItem();
        if (databaseItem) {
            databaseItem->expandCollections();
        }
    }
}
