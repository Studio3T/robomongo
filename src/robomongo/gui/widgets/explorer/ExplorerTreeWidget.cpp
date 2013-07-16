#include "robomongo/gui/widgets/explorer/ExplorerTreeWidget.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QApplication>
#include <QtGui>
#include <QMessageBox>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/explorer/EditIndexDialog.h"
#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/dialogs/FunctionTextEditor.h"
#include "robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"
#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"
#include "robomongo/gui/dialogs/CreateUserDialog.h"
#include "robomongo/shell/db/json.h"

namespace
{
	template<typename type_t>
	type_t get_item(QList<QTreeWidgetItem*> items)
	{
		type_t result = NULL;
		if (items.count() == 1)
		{
			QTreeWidgetItem *item = items[0];
			if (item){
				result= dynamic_cast<type_t>(item);
			}
		}
		return result;
	}
}

namespace Robomongo
{

    ExplorerTreeWidget::ExplorerTreeWidget(QWidget *parent) : QTreeWidget(parent)
    {
    #if defined(Q_OS_MAC)
        setAttribute(Qt::WA_MacShowFocusRect, false);
    #endif
        setContextMenuPolicy(Qt::DefaultContextMenu);
        setObjectName("explorerTree");                           

        QAction *addIndex = new QAction("Add Index", this);
        connect(addIndex, SIGNAL(triggered()), SLOT(ui_addIndex()));

        QAction *addIndexGui = new QAction("Add Index GUI", this);
        connect(addIndexGui, SIGNAL(triggered()), SLOT(ui_addIndexGui()));

        QAction *dropIndex = new QAction("Drop Index", this);
        connect(dropIndex, SIGNAL(triggered()), SLOT(ui_dropIndex()));

        QAction *reIndex = new QAction("Rebuild Indexes", this);
        connect(reIndex, SIGNAL(triggered()), SLOT(ui_reIndex()));

        QAction *viewIndex = new QAction("View Indexes", this);
        connect(viewIndex, SIGNAL(triggered()), SLOT(ui_viewIndex()));

        QAction *refreshIndex = new QAction("Refresh Indexes", this);
        connect(refreshIndex, SIGNAL(triggered()), SLOT(ui_refreshIndex()));

        _indexDirContextMenu = new QMenu(this);
        _indexDirContextMenu->addAction(viewIndex);
        _indexDirContextMenu->addAction(addIndex);
        _indexDirContextMenu->addAction(addIndexGui);
        _indexDirContextMenu->addAction(dropIndex);
        _indexDirContextMenu->addAction(reIndex);
        _indexDirContextMenu->addAction(refreshIndex);        


        QAction *createCollection = new QAction("Create Collection", this);
        connect(createCollection, SIGNAL(triggered()), SLOT(ui_createCollection()));

        QAction *dbCollectionsStats = new QAction("Collections Statistics", this);
        connect(dbCollectionsStats, SIGNAL(triggered()), SLOT(ui_dbCollectionsStatistics()));

        QAction *refreshCollections = new QAction("Refresh", this);
        connect(refreshCollections, SIGNAL(triggered()), SLOT(ui_refreshCollections()));

        _collectionCategoryContextMenu = new QMenu(this);
        _collectionCategoryContextMenu->addAction(dbCollectionsStats);
        _collectionCategoryContextMenu->addAction(createCollection);
        _collectionCategoryContextMenu->addSeparator();
        _collectionCategoryContextMenu->addAction(refreshCollections);


        QAction *refreshUsers = new QAction("Refresh", this);
        connect(refreshUsers, SIGNAL(triggered()), SLOT(ui_refreshUsers()));

        QAction *viewUsers = new QAction("View Users", this);
        connect(viewUsers, SIGNAL(triggered()), SLOT(ui_viewUsers()));

        QAction *addUser = new QAction("Add User", this);
        connect(addUser, SIGNAL(triggered()), SLOT(ui_addUser()));

        _usersCategoryContextMenu = new QMenu(this);
        _usersCategoryContextMenu->addAction(viewUsers);
        _usersCategoryContextMenu->addAction(addUser);
        _usersCategoryContextMenu->addSeparator();
        _usersCategoryContextMenu->addAction(refreshUsers);


        QAction *refreshFunctions = new QAction("Refresh", this);
        connect(refreshFunctions, SIGNAL(triggered()), SLOT(ui_refreshFunctions()));

        QAction *viewFunctions = new QAction("View Functions", this);
        connect(viewFunctions, SIGNAL(triggered()), SLOT(ui_viewFunctions()));

        QAction *addFunction = new QAction("Add Function", this);
        connect(addFunction, SIGNAL(triggered()), SLOT(ui_addFunction()));

        _functionsCategoryContextMenu = new QMenu(this);
        _functionsCategoryContextMenu->addAction(viewFunctions);
        _functionsCategoryContextMenu->addAction(addFunction);
        _functionsCategoryContextMenu->addSeparator();
        _functionsCategoryContextMenu->addAction(refreshFunctions);

        QAction *deleteIndex = new QAction("Delete index", this);
        connect(deleteIndex, SIGNAL(triggered()), SLOT(ui_deleteIndex()));

        _indexContextMenu = new QMenu(this);
        _indexContextMenu->addAction(deleteIndex);
    }

    void ExplorerTreeWidget::contextMenuEvent(QContextMenuEvent *event)
    {
        QTreeWidgetItem *item = itemAt(event->pos());
        if (item){
			ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
            if (serverItem) {
                serverItem->showContextMenuAtPos(mapToGlobal(event->pos()));
				return;
			}

			ExplorerCollectionTreeItem *collectionItem = dynamic_cast<ExplorerCollectionTreeItem *>(item);
			if (collectionItem) {
				collectionItem->showContextMenuAtPos(mapToGlobal(event->pos()));
				return;
			}

			ExplorerDatabaseTreeItem *databaseItem = dynamic_cast<ExplorerDatabaseTreeItem *>(item);
			if (databaseItem) {
				databaseItem->showContextMenuAtPos(mapToGlobal(event->pos()));
				return;
			}

			ExplorerUserTreeItem *userItem = dynamic_cast<ExplorerUserTreeItem *>(item);
			if (userItem) {
				userItem->showContextMenuAtPos(mapToGlobal(event->pos()));
				return;
			}

			ExplorerFunctionTreeItem *functionItem = dynamic_cast<ExplorerFunctionTreeItem *>(item);
			if (functionItem) {
				functionItem->showContextMenuAtPos(mapToGlobal(event->pos()));
				return;
			}

			ExplorerDatabaseCategoryTreeItem *categoryItem = dynamic_cast<ExplorerDatabaseCategoryTreeItem *>(item);
			if (categoryItem) {
				if (categoryItem->category() == Collections) {
					_collectionCategoryContextMenu->exec(mapToGlobal(event->pos()));
				} else if (categoryItem->category() == Users) {
					_usersCategoryContextMenu->exec(mapToGlobal(event->pos()));
				} else if (categoryItem->category() == Functions) {
					_functionsCategoryContextMenu->exec(mapToGlobal(event->pos()));
				}
			}

            ExplorerCollectionDirIndexesTreeItem *indDir = dynamic_cast<ExplorerCollectionDirIndexesTreeItem *>(item);
            if(indDir){
                _indexDirContextMenu->exec(mapToGlobal(event->pos()));
            }

            ExplorerCollectionIndexesTreeItem *ind = dynamic_cast<ExplorerCollectionIndexesTreeItem *>(item);
            if(ind&&ind->text(0)!="_id"){
                _indexContextMenu->exec(mapToGlobal(event->pos()));
            }
		}
    }

    ExplorerServerTreeItem *ExplorerTreeWidget::selectedServerItem()
    {
        return get_item<ExplorerServerTreeItem*>(selectedItems());
    }

    ExplorerCollectionDirIndexesTreeItem *ExplorerTreeWidget::selectedCollectionDirIndexItem()
    {
        return get_item<ExplorerCollectionDirIndexesTreeItem*>(selectedItems());
    }

    ExplorerCollectionTreeItem *ExplorerTreeWidget::selectedCollectionItem()
    {
        return get_item<ExplorerCollectionTreeItem*>(selectedItems());
    }

    ExplorerUserTreeItem *ExplorerTreeWidget::selectedUserItem()
    {
        return get_item<ExplorerUserTreeItem*>(selectedItems());
    }

    ExplorerFunctionTreeItem *ExplorerTreeWidget::selectedFunctionItem()
    {
        return get_item<ExplorerFunctionTreeItem*>(selectedItems());
    }

    ExplorerDatabaseTreeItem *ExplorerTreeWidget::selectedDatabaseItem()
    {
        return get_item<ExplorerDatabaseTreeItem*>(selectedItems());
    }

    ExplorerDatabaseCategoryTreeItem *ExplorerTreeWidget::selectedDatabaseCategoryItem()
    {
        return get_item<ExplorerDatabaseCategoryTreeItem*>(selectedItems());
    }

    void ExplorerTreeWidget::openCurrentCollectionShell(const QString &script, bool execute,
        const CursorPosition &cursor)
    {
        ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
        if (!collectionItem){
            ExplorerCollectionDirIndexesTreeItem *collectionDirIndexes = selectedCollectionDirIndexItem();
            if(collectionDirIndexes)
            {
                collectionItem = dynamic_cast<ExplorerCollectionTreeItem*>(collectionDirIndexes->parent());
            }
        }
        if (collectionItem){
            MongoCollection *collection = collectionItem->collection();
            QString query = AppRegistry::instance().app()->buildCollectionQuery(collection->name(), script);
            AppRegistry::instance().app()->openShell(collection->database(), query, execute, collection->name(), cursor);
        }
    }

    void ExplorerTreeWidget::openDatabaseShell(MongoDatabase *database, const QString &script, bool execute, const CursorPosition &cursor)
    {
		AppRegistry::instance().app()->openShell(database, script, execute, database->name(), cursor);
    }

    void ExplorerTreeWidget::ui_addIndex()
    {
        openCurrentCollectionShell(
            "ensureIndex({ \"<field>\" : 1 }); \n"
            "\n"
            "// options: \n"
            "// { unique : true }   - A unique index causes MongoDB to reject all documents that contain a duplicate value for the indexed field. \n"
            "// { sparse : true }   - Sparse indexes only contain entries for documents that have the indexed field. \n"
            "// { dropDups : true } - Sparse indexes only contain entries for documents that have the indexed field. \n"
        , false);
    }

    void ExplorerTreeWidget::ui_addIndexGui()
    {
        ExplorerCollectionDirIndexesTreeItem *item = selectedCollectionDirIndexItem();
        if (item){
            ExplorerCollectionTreeItem *const parent = dynamic_cast<ExplorerCollectionTreeItem *const>(item->parent());
            if(parent){
                EditIndexDialog dlg(this,parent);
                int result = dlg.exec();
                if (result == QDialog::Accepted) {
                    (static_cast<ExplorerDatabaseTreeItem *const>(parent->QObject::parent()))->enshureIndex(parent,dlg.getInputText(),dlg.isUnique(),dlg.isBackGround(),dlg.isDropDuplicates());
                }
            }
        }
    }

    void ExplorerTreeWidget::ui_reIndex()
    {
        openCurrentCollectionShell("reIndex()", false);
    }

    void ExplorerTreeWidget::ui_dropIndex()
    {
        openCurrentCollectionShell(
            "dropIndex({ \"<field>\" : 1 });"
            , false);
    }

    void ExplorerTreeWidget::ui_dbCollectionsStatistics()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			openDatabaseShell(categoryItem->databaseItem()->database(), "db.printCollectionStats()");
		}
    } 

    void ExplorerTreeWidget::ui_refreshUsers()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			categoryItem->databaseItem()->expandUsers();
		}
    }

    void ExplorerTreeWidget::ui_refreshFunctions()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			categoryItem->databaseItem()->expandFunctions();
		}
    }

    void ExplorerTreeWidget::ui_viewUsers()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			openDatabaseShell(categoryItem->databaseItem()->database(),"db.system.users.find()");
		}
    }

    void ExplorerTreeWidget::ui_viewFunctions()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			openDatabaseShell(categoryItem->databaseItem()->database(),
				"db.system.js.find()");
		}
    }

    void ExplorerTreeWidget::ui_createCollection()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem)
		{
			ExplorerDatabaseTreeItem *databaseItem = categoryItem->databaseItem();
			if (databaseItem){
				CreateDatabaseDialog dlg(databaseItem->database()->server()->connectionRecord()->getFullAddress(),
										 databaseItem->database()->name());
				dlg.setWindowTitle("Create Collection");
				dlg.setOkButtonText("&Create");
				dlg.setInputLabelText("Collection Name:");
				int result = dlg.exec();
				if (result == QDialog::Accepted) {
					databaseItem->database()->createCollection(dlg.databaseName());
					// refresh list of databases
					databaseItem->expandCollections();
				}
			}
		}
    }

    void ExplorerTreeWidget::ui_addUser()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			ExplorerDatabaseTreeItem *databaseItem = categoryItem->databaseItem();
			if(databaseItem){
				CreateUserDialog dlg(databaseItem->database()->server()->connectionRecord()->getFullAddress(),
									 databaseItem->database()->name());
				int result = dlg.exec();
				if (result == QDialog::Accepted) {
					MongoUser user = dlg.user();
					databaseItem->database()->createUser(user, false);
					// refresh list of users
					databaseItem->expandUsers();
				}
			}
		}
    }

    void ExplorerTreeWidget::ui_addFunction()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			ExplorerDatabaseTreeItem *databaseItem = categoryItem->databaseItem();
			FunctionTextEditor dlg(databaseItem->database()->server()->connectionRecord()->getFullAddress(),
								   databaseItem->database()->name(), MongoFunction());
			dlg.setWindowTitle("Create Function");
			dlg.setCode(
				"function() {\n"
				"    // write your code here\n"
				"}");
			dlg.setCursorPosition(1, 4);
			int result = dlg.exec();
			if (result == QDialog::Accepted) {
				MongoFunction function = dlg.function();
				databaseItem->database()->createFunction(function);
				// refresh list of functions
				databaseItem->expandFunctions();
			}
		}
    }

    void ExplorerTreeWidget::ui_refreshCollections()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			categoryItem->databaseItem()->expandCollections();}
    }

    void ExplorerTreeWidget::ui_deleteIndex()
    {
        ExplorerCollectionIndexesTreeItem *ind = get_item<ExplorerCollectionIndexesTreeItem*>(selectedItems());
        if(ind){
            ExplorerCollectionDirIndexesTreeItem *parent = dynamic_cast<ExplorerCollectionDirIndexesTreeItem *>(ind->parent());           
            if(parent){
                ExplorerCollectionTreeItem *grandParent = dynamic_cast<ExplorerCollectionTreeItem *>(parent->parent());
                if(grandParent){
                    grandParent->deleteIndex(ind);
                }
            }
        }
    }

    void ExplorerTreeWidget::ui_viewIndex()
    {
        openCurrentCollectionShell("getIndexes()");
    }

    void ExplorerTreeWidget::ui_refreshIndex()
    {
        ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
        if (!collectionItem){
            ExplorerCollectionDirIndexesTreeItem *collectionDirIndexes = selectedCollectionDirIndexItem();
            if(collectionDirIndexes)
            {
                collectionItem = dynamic_cast<ExplorerCollectionTreeItem*>(collectionDirIndexes->parent());
            }
        }
        if(collectionItem){
            collectionItem->expand();
        }
    }
}
