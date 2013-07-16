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

        QAction *openDbShellAction = new QAction("Open Shell", this);
        openDbShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
        connect(openDbShellAction, SIGNAL(triggered()), SLOT(ui_dbOpenShell()));

        QAction *dbStats = new QAction("Database Statistics", this);
        connect(dbStats, SIGNAL(triggered()), SLOT(ui_dbStatistics()));

        QAction *dbDrop = new QAction("Drop Database", this);
        connect(dbDrop, SIGNAL(triggered()), SLOT(ui_dbDrop()));

        QAction *dbRepair = new QAction("Repair Database", this);
        connect(dbRepair, SIGNAL(triggered()), SLOT(ui_dbRepair()));

        QAction *refreshDatabase = new QAction("Refresh", this);
        connect(refreshDatabase, SIGNAL(triggered()), SLOT(ui_refreshDatabase()));

        _databaseContextMenu = new QMenu(this);
        _databaseContextMenu->addAction(openDbShellAction);
        _databaseContextMenu->addAction(refreshDatabase);
        _databaseContextMenu->addSeparator();
        _databaseContextMenu->addAction(dbStats);
        _databaseContextMenu->addSeparator();
        _databaseContextMenu->addAction(dbRepair);
        _databaseContextMenu->addAction(dbDrop);

        QAction *addDocument = new QAction("Insert Document", this);
        connect(addDocument, SIGNAL(triggered()), SLOT(ui_addDocument()));

        QAction *updateDocument = new QAction("Update Documents", this);
        connect(updateDocument, SIGNAL(triggered()), SLOT(ui_updateDocument()));

        QAction *removeDocument = new QAction("Remove Documents", this);
        connect(removeDocument, SIGNAL(triggered()), SLOT(ui_removeDocument()));

        QAction *removeAllDocuments = new QAction("Remove All Documents", this);
        connect(removeAllDocuments, SIGNAL(triggered()), SLOT(ui_removeAllDocuments()));

        QAction *collectionStats = new QAction("Statistics", this);
        connect(collectionStats, SIGNAL(triggered()), SLOT(ui_collectionStatistics()));

        QAction *storageSize = new QAction("Storage Size", this);
        connect(storageSize, SIGNAL(triggered()), SLOT(ui_storageSize()));

        QAction *totalIndexSize = new QAction("Total Index Size", this);
        connect(totalIndexSize, SIGNAL(triggered()), SLOT(ui_totalIndexSize()));

        QAction *totalSize = new QAction("Total Size", this);
        connect(totalSize, SIGNAL(triggered()), SLOT(ui_totalSize()));

        QAction *shardVersion = new QAction("Shard Version", this);
        connect(shardVersion, SIGNAL(triggered()), SLOT(ui_shardVersion()));

        QAction *shardDistribution = new QAction("Shard Distribution", this);
        connect(shardDistribution, SIGNAL(triggered()), SLOT(ui_shardDistribution()));

        QAction *dropCollection = new QAction("Drop Collection", this);
        connect(dropCollection, SIGNAL(triggered()), SLOT(ui_dropCollection()));

        QAction *renameCollection = new QAction("Rename Collection", this);
        connect(renameCollection, SIGNAL(triggered()), SLOT(ui_renameCollection()));

        QAction *duplicateCollection = new QAction("Duplicate Collection", this);
        connect(duplicateCollection, SIGNAL(triggered()), SLOT(ui_duplicateCollection()));

        QAction *viewCollection = new QAction("View Documents", this);
        connect(viewCollection, SIGNAL(triggered()), SLOT(ui_viewCollection()));

        _collectionContextMenu = new QMenu(this);
        _collectionContextMenu->addAction(viewCollection);
        _collectionContextMenu->addSeparator();
        _collectionContextMenu->addAction(addDocument);
        _collectionContextMenu->addAction(updateDocument);
        _collectionContextMenu->addAction(removeDocument);
        _collectionContextMenu->addAction(removeAllDocuments);
        _collectionContextMenu->addSeparator();
        _collectionContextMenu->addAction(renameCollection);
        _collectionContextMenu->addAction(duplicateCollection);
        _collectionContextMenu->addAction(dropCollection);
        _collectionContextMenu->addSeparator();
        _collectionContextMenu->addAction(collectionStats);
        _collectionContextMenu->addSeparator();
        _collectionContextMenu->addAction(shardVersion);
        _collectionContextMenu->addAction(shardDistribution);

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


        QAction *dropFunction = new QAction("Remove Function", this);
        connect(dropFunction, SIGNAL(triggered()), SLOT(ui_dropFunction()));

        QAction *editFunction = new QAction("Edit Function", this);
        connect(editFunction, SIGNAL(triggered()), SLOT(ui_editFunction()));

        _functionContextMenu = new QMenu(this);
        _functionContextMenu->addAction(editFunction);
        _functionContextMenu->addAction(dropFunction);


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
				_collectionContextMenu->exec(mapToGlobal(event->pos()));
				return;
			}

			ExplorerDatabaseTreeItem *databaseItem = dynamic_cast<ExplorerDatabaseTreeItem *>(item);
			if (databaseItem) {
				_databaseContextMenu->exec(mapToGlobal(event->pos()));
				return;
			}

			ExplorerUserTreeItem *userItem = dynamic_cast<ExplorerUserTreeItem *>(item);
			if (userItem) {
				userItem->showContextMenuAtPos(mapToGlobal(event->pos()));
				return;
			}

			ExplorerFunctionTreeItem *functionItem = dynamic_cast<ExplorerFunctionTreeItem *>(item);
			if (functionItem) {
				_functionContextMenu->exec(mapToGlobal(event->pos()));
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

    void ExplorerTreeWidget::openCurrentDatabaseShell(const QString &script, bool execute,
                                                      const CursorPosition &cursor)
    {
        ExplorerDatabaseTreeItem *collectionItem = selectedDatabaseItem();
        if (collectionItem){
			MongoDatabase *database = collectionItem->database();
			openDatabaseShell(database, script, execute, cursor);
		}
    }

    void ExplorerTreeWidget::openDatabaseShell(MongoDatabase *database, const QString &script, bool execute, const CursorPosition &cursor)
    {
		AppRegistry::instance().app()->openShell(database, script, execute, database->name(), cursor);
    } 

    void ExplorerTreeWidget::ui_addDocument()
    {
        ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
        if (collectionItem){
			MongoCollection *collection = collectionItem->collection();
			MongoDatabase *database = collection->database();
			MongoServer *server = database->server();
			ConnectionSettings *settings = server->connectionRecord();

			DocumentTextEditor editor(settings->getFullAddress(), database->name(),
									  collection->name(), "{\n    \n}");

			editor.setCursorPosition(1, 4);
			editor.setWindowTitle("Insert Document");
			int result = editor.exec();
			activateWindow();

			if (result == QDialog::Accepted) {
				mongo::BSONObj obj = editor.bsonObj();
				server->insertDocument(obj, database->name(), collection->name());
			}
		}
    }

    void ExplorerTreeWidget::ui_removeDocument()
    {
        openCurrentCollectionShell(
            "remove({ '' : '' });"
            , false, CursorPosition(0, -10));
    }

    void ExplorerTreeWidget::ui_removeAllDocuments()
    {
        ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
        if (collectionItem){
			MongoCollection *collection = collectionItem->collection();
			MongoDatabase *database = collection->database();
			// Ask user
			int answer = QMessageBox::question(this,
					"Remove All Documents",
					QString("Remove all documents from <b>%1</b> collection?").arg(collection->name()),
					QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

			if (answer == QMessageBox::Yes){
				MongoServer *server = database->server();
				mongo::BSONObjBuilder builder;
				mongo::BSONObj bsonQuery = builder.obj();
				mongo::Query query(bsonQuery);
				server->removeDocuments(query, database->name(), collection->name(), false);
			}
		}
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

    void ExplorerTreeWidget::ui_updateDocument()
    {
        openCurrentCollectionShell(
            "update(\n"
            "    // query \n"
            "    {\n"
            "        \"key\" : \"value\"\n"
            "    },\n"
            "    \n"
            "    // update \n"
            "    {\n"
            "    },\n"
            "    \n"
            "    // options \n"
            "    {\n"
            "        \"multi\" : false,  // update only one document \n"
            "        \"upsert\" : false  // insert a new document, if no existing document match the query \n"
            "    }\n"
            ");", false);
    }

    void ExplorerTreeWidget::ui_collectionStatistics()
    {
        openCurrentCollectionShell("stats()");
    }

    void ExplorerTreeWidget::ui_dropCollection()
    {
        ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
        if (collectionItem){
			MongoCollection *collection = collectionItem->collection();
			MongoDatabase *database = collection->database();
			MongoServer *server = database->server();
			ConnectionSettings *settings = server->connectionRecord();

			// Ask user
			int answer = QMessageBox::question(this,
					"Drop Collection",
					QString("Drop <b>%1</b> collection?").arg(collection->name()),
					QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

			if (answer == QMessageBox::Yes){
				database->dropCollection(collection->name());
				database->loadCollections();
			}
		}
        //openCurrentCollectionShell("drop()", false);
    }

    void ExplorerTreeWidget::ui_duplicateCollection()
    {
        ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
        if (collectionItem){
			MongoCollection *collection = collectionItem->collection();
			MongoDatabase *database = collection->database();
			MongoServer *server = database->server();
			ConnectionSettings *settings = server->connectionRecord();

			CreateDatabaseDialog dlg(settings->getFullAddress(),
									 database->name(),
									 collection->name());
			dlg.setWindowTitle("Duplicate Collection");
			dlg.setOkButtonText("&Duplicate");
			dlg.setInputLabelText("New Collection Name:");
			dlg.setInputText(collection->name() + "_copy");
			int result = dlg.exec();

			if (result == QDialog::Accepted) {
				database->duplicateCollection(collection->name(), dlg.databaseName());

				// refresh list of collections
				database->loadCollections();
			}
		}
    }

    void ExplorerTreeWidget::ui_renameCollection()
    {
        ExplorerCollectionTreeItem *collectionItem = selectedCollectionItem();
        if (collectionItem){
			MongoCollection *collection = collectionItem->collection();
			MongoDatabase *database = collection->database();
			MongoServer *server = database->server();
			ConnectionSettings *settings = server->connectionRecord();

			CreateDatabaseDialog dlg(settings->getFullAddress(),
									 database->name(),
									 collection->name());
			dlg.setWindowTitle("Rename Collection");
			dlg.setOkButtonText("&Rename");
			dlg.setInputLabelText("New Collection Name:");
			dlg.setInputText(collection->name());
			int result = dlg.exec();

			if (result == QDialog::Accepted) {
				database->renameCollection(collection->name(), dlg.databaseName());
				// refresh list of collections
				database->loadCollections();
			}
		}
    }

    void ExplorerTreeWidget::ui_viewCollection()
    {
        openCurrentCollectionShell("find()");
    }

    void ExplorerTreeWidget::ui_storageSize()
    {
        openCurrentCollectionShell("storageSize()");
    }

    void ExplorerTreeWidget::ui_totalIndexSize()
    {
        openCurrentCollectionShell("totalIndexSize()");
    }

    void ExplorerTreeWidget::ui_totalSize()
    {
        openCurrentCollectionShell("totalSize()");
    }

    void ExplorerTreeWidget::ui_shardVersion()
    {
        openCurrentCollectionShell("getShardVersion()");
    }

    void ExplorerTreeWidget::ui_shardDistribution()
    {
        openCurrentCollectionShell("getShardDistribution()");
    }

    void ExplorerTreeWidget::ui_dbStatistics()
    {
        openCurrentDatabaseShell("db.stats()");
    }

    void ExplorerTreeWidget::ui_dbDrop()
    {
        ExplorerDatabaseTreeItem *dbItem = selectedDatabaseItem();
        if (dbItem){
			MongoDatabase *database = dbItem->database();
			// Ask user
			int answer = QMessageBox::question(this,
					"Drop Database",
					QString("Drop <b>%1</b> database?").arg(database->name()),
					QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);
			if (answer == QMessageBox::Yes){
				MongoServer *server = database->server();
				server->dropDatabase(database->name());
				server->loadDatabases(); // refresh list of databases
			}
		}
    }

    void ExplorerTreeWidget::ui_dbCollectionsStatistics()
    {
        ExplorerDatabaseCategoryTreeItem *categoryItem = selectedDatabaseCategoryItem();
        if (categoryItem){
			openDatabaseShell(categoryItem->databaseItem()->database(), "db.printCollectionStats()");
		}
    }

    void ExplorerTreeWidget::ui_dbRepair()
    {
        openCurrentDatabaseShell("db.repairDatabase()", false);
    }

    void ExplorerTreeWidget::ui_dbOpenShell()
    {
        openCurrentDatabaseShell("");
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

    void ExplorerTreeWidget::ui_refreshDatabase()
    {
        ExplorerDatabaseTreeItem *databaseItem = selectedDatabaseItem();
        if (databaseItem){
			databaseItem->expandCollections();
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

    void ExplorerTreeWidget::ui_editFunction()
    {
        ExplorerFunctionTreeItem *functionItem = selectedFunctionItem();
        if (functionItem){
			MongoFunction function = functionItem->function();
			MongoDatabase *database = functionItem->database();
			MongoServer *server = database->server();
			QString name = function.name();

			FunctionTextEditor dlg(server->connectionRecord()->getFullAddress(),
								 database->name(),
								 function);
			dlg.setWindowTitle("Edit Function");
			int result = dlg.exec();

			if (result == QDialog::Accepted) {

				MongoFunction editedFunction = dlg.function();
				database->updateFunction(name, editedFunction);

				// refresh list of functions
				database->loadFunctions();
			}
		}
    }

    void ExplorerTreeWidget::ui_dropFunction()
    {
        ExplorerFunctionTreeItem *functionItem = selectedFunctionItem();
        if (!functionItem)
            return;

        MongoFunction function = functionItem->function();
        MongoDatabase *database = functionItem->database();

        // Ask user
        int answer = QMessageBox::question(this,
                "Remove Function",
                QString("Remove <b>%1</b> function?").arg(function.name()),
                QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        if (answer != QMessageBox::Yes)
            return;

        database->dropFunction(function.name());
        database->loadFunctions(); // refresh list of functions
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
