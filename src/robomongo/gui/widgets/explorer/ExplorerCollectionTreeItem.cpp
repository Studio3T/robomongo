#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/gui/widgets/explorer/EditIndexDialog.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"
#include "robomongo/gui/dialogs/CopyCollectionDialog.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/DialogUtils.h"

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

namespace
{
    const char *tooltipTemplate =
        "%s "
        "<table>"
        "<tr><td>Count:</td> <td><b>&nbsp;&nbsp;%d</b></td></tr>"
        "<tr><td>Size:</td><td><b>&nbsp;&nbsp;%s</b></td></tr>"
        "</table>"
        ;
}

namespace Robomongo
{
    R_REGISTER_EVENT(CollectionIndexesLoadingEvent)
    const QString ExplorerCollectionDirIndexesTreeItem::labelText = "Indexes";

    ExplorerCollectionDirIndexesTreeItem::ExplorerCollectionDirIndexesTreeItem(QTreeWidgetItem *parent)
        :BaseClass(parent)
    {
        _addIndexAction = new QAction(this);
        VERIFY(connect(_addIndexAction, SIGNAL(triggered()), SLOT(ui_addIndex())));

        _addIndexGuiAction = new QAction(this);
        VERIFY(connect(_addIndexGuiAction, SIGNAL(triggered()), SLOT(ui_addIndexGui())));

        _dropIndexAction = new QAction(this);
        VERIFY(connect(_dropIndexAction, SIGNAL(triggered()), SLOT(ui_dropIndex())));

        _reIndexAction = new QAction(this);
        VERIFY(connect(_reIndexAction, SIGNAL(triggered()), SLOT(ui_reIndex())));

        _viewIndexAction = new QAction(this);
        VERIFY(connect(_viewIndexAction, SIGNAL(triggered()), SLOT(ui_viewIndex())));

        _refreshIndexAction = new QAction(this);
        VERIFY(connect(_refreshIndexAction, SIGNAL(triggered()), SLOT(ui_refreshIndex())));

        BaseClass::_contextMenu->addAction(_viewIndexAction);
        //BaseClass::_contextMenu->addAction(addIndex);
        BaseClass::_contextMenu->addAction(_addIndexGuiAction);
        //BaseClass::_contextMenu->addAction(dropIndex);
        BaseClass::_contextMenu->addAction(_reIndexAction);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(_refreshIndexAction);      

        setIcon(0, Robomongo::GuiRegistry::instance().folderIcon());

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        
        retranslateUI();
    }
    
    void ExplorerCollectionDirIndexesTreeItem::retranslateUI()
    {
        _addIndexAction->setText(tr("Add Index..."));
        _addIndexGuiAction->setText(tr("Add Index..."));
        _dropIndexAction->setText(tr("Drop Index..."));
        _reIndexAction->setText(tr("Rebuild Indexes..."));
        _viewIndexAction->setText(tr("View Indexes"));
        _refreshIndexAction->setText(tr("Refresh"));
        setText(0, tr("Indexes"));
    }

    void ExplorerCollectionDirIndexesTreeItem::expand()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (!par)
            return;

        par->expand();
    }

    void ExplorerCollectionDirIndexesTreeItem::ui_viewIndex()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (par) {
            par->openCurrentCollectionShell("getIndexes()");
        }
    }

    void ExplorerCollectionDirIndexesTreeItem::ui_refreshIndex()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (par) {
            par->expand();
        }
    }

    void ExplorerCollectionDirIndexesTreeItem::ui_addIndex()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (par) {
            par->openCurrentCollectionShell(
                "ensureIndex({ \"<field>\" : 1 }); \n"
                "\n"
                "// options: \n"
                "// { unique : true }   - " + tr("A unique index causes MongoDB to reject all documents that contain a duplicate value for the indexed field.") + " \n"
                "// { sparse : true }   - " + tr("Sparse indexes only contain entries for documents that have the indexed field.") + " \n"
                "// { dropDups : true } - " + tr("") + "http://docs.mongodb.org/manual/core/index-creation/#index-creation-duplicate-dropping \n"
                , false);
        }
    }

    void ExplorerCollectionDirIndexesTreeItem::ui_addIndexGui()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *const>(parent());
        if (!par)
            return;
        EnsureIndexInfo fakeInfo(par->collection()->info(),"");
        EditIndexDialog dlg(fakeInfo , QtUtils::toQString(par->databaseItem()->database()->name()),QtUtils::toQString(par->databaseItem()->database()->server()->connectionRecord()->getFullAddress()), treeWidget());
        int result = dlg.exec();
        if (result != QDialog::Accepted)
            return;

        ExplorerDatabaseTreeItem *databaseTreeItem = static_cast<ExplorerDatabaseTreeItem *>(par->databaseItem());
        if (!databaseTreeItem)
            return;

        databaseTreeItem->enshureIndex(par,fakeInfo, dlg.info());
    }

    void ExplorerCollectionDirIndexesTreeItem::ui_reIndex()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (par) {
            par->openCurrentCollectionShell("reIndex()", false);
        }
    }

    void ExplorerCollectionDirIndexesTreeItem::ui_dropIndex()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (par) {
            par->openCurrentCollectionShell("dropIndex({ \"<field>\" : 1 });", false);
        }
    }

    ExplorerCollectionIndexesTreeItem::ExplorerCollectionIndexesTreeItem(ExplorerCollectionDirIndexesTreeItem *parent,const EnsureIndexInfo &info)
        : BaseClass(parent),_info(info)
    {
        _deleteIndexAction = new QAction(this);
        connect(_deleteIndexAction, SIGNAL(triggered()), SLOT(ui_dropIndex()));
        _editIndexAction = new QAction(this);
        connect(_editIndexAction, SIGNAL(triggered()), SLOT(ui_edit()));

        BaseClass::_contextMenu->addAction(_editIndexAction);
        BaseClass::_contextMenu->addAction(_deleteIndexAction);

        setText(0, QtUtils::toQString(_info._name));
        setIcon(0, Robomongo::GuiRegistry::instance().indexIcon());

        retranslateUI();
    }
    
    void ExplorerCollectionIndexesTreeItem::retranslateUI()
    {
        _deleteIndexAction->setText(tr("Drop Index..."));
        _editIndexAction->setText(tr("Edit Index..."));
    }

    void ExplorerCollectionIndexesTreeItem::ui_dropIndex()
    {
        // Ask user
        int answer = utils::questionDialog(treeWidget(),tr("Drop"),tr("Index"),text(0));

        if (answer != QMessageBox::Yes)
            return;

        ExplorerCollectionDirIndexesTreeItem *par = dynamic_cast<ExplorerCollectionDirIndexesTreeItem *>(parent());
        if (!par)
            return;
        
        ExplorerCollectionTreeItem *grandParent = dynamic_cast<ExplorerCollectionTreeItem *>(par->parent());
        if (!grandParent)
            return;

        grandParent->dropIndex(this);
    }

    void ExplorerCollectionIndexesTreeItem::ui_edit()
    {
        ExplorerCollectionDirIndexesTreeItem *par = dynamic_cast<ExplorerCollectionDirIndexesTreeItem *>(parent());           
        if (par) {
            ExplorerCollectionTreeItem *grPar = dynamic_cast<ExplorerCollectionTreeItem *const>(par->parent());
            if (!par)
                return;

            EditIndexDialog dlg(_info, QtUtils::toQString(grPar->databaseItem()->database()->name()), QtUtils::toQString(grPar->databaseItem()->database()->server()->connectionRecord()->getFullAddress()), treeWidget());
            int result = dlg.exec();
            if (result != QDialog::Accepted)
                return;

            ExplorerDatabaseTreeItem *databaseTreeItem = static_cast<ExplorerDatabaseTreeItem *>(grPar->databaseItem());
            if (!databaseTreeItem)
                return;

            databaseTreeItem->enshureIndex(grPar, _info, dlg.info());
        }
    }

    ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(QTreeWidgetItem *parent, ExplorerDatabaseTreeItem *databaseItem, MongoCollection *collection) :
        BaseClass(parent), _collection(collection), _databaseItem(databaseItem)
    {
        _addDocumentAction = new QAction(this);
        VERIFY(connect(_addDocumentAction, SIGNAL(triggered()), SLOT(ui_addDocument())));

        _updateDocumentAction = new QAction(this);
        VERIFY(connect(_updateDocumentAction, SIGNAL(triggered()), SLOT(ui_updateDocument())));
        
        _removeDocumentAction = new QAction(this);
        VERIFY(connect(_removeDocumentAction, SIGNAL(triggered()), SLOT(ui_removeDocument())));
        _removeAllDocumentsAction = new QAction(this);
        VERIFY(connect(_removeAllDocumentsAction, SIGNAL(triggered()), SLOT(ui_removeAllDocuments())));

        _collectionStatsAction = new QAction(this);
        VERIFY(connect(_collectionStatsAction, SIGNAL(triggered()), SLOT(ui_collectionStatistics())));

        _storageSizeAction = new QAction(this);
        VERIFY(connect(_storageSizeAction, SIGNAL(triggered()), SLOT(ui_storageSize())));
        _totalIndexSizeAction = new QAction(this);
        VERIFY(connect(_totalIndexSizeAction, SIGNAL(triggered()), SLOT(ui_totalIndexSize())));
        _totalSizeAction = new QAction(this);
        VERIFY(connect(_totalSizeAction, SIGNAL(triggered()), SLOT(ui_totalSize())));
        
        _shardVersionAction = new QAction(this);
        VERIFY(connect(_shardVersionAction, SIGNAL(triggered()), SLOT(ui_shardVersion())));
        _shardDistributionAction = new QAction(this);
        VERIFY(connect(_shardDistributionAction, SIGNAL(triggered()), SLOT(ui_shardDistribution())));

        _dropCollectionAction = new QAction(this);
        VERIFY(connect(_dropCollectionAction, SIGNAL(triggered()), SLOT(ui_dropCollection())));
        _renameCollectionAction = new QAction(this);
        VERIFY(connect(_renameCollectionAction, SIGNAL(triggered()), SLOT(ui_renameCollection())));
        _duplicateCollectionAction = new QAction(this);
        VERIFY(connect(_duplicateCollectionAction, SIGNAL(triggered()), SLOT(ui_duplicateCollection())));
        _copyCollectionToDiffrentServerAction = new QAction(this);
        VERIFY(connect(_copyCollectionToDiffrentServerAction, SIGNAL(triggered()), SLOT(ui_copyToCollectionToDiffrentServer())));
        _viewCollectionAction = new QAction(this);
        VERIFY(connect(_viewCollectionAction, SIGNAL(triggered()), SLOT(ui_viewCollection())));

        BaseClass::_contextMenu->addAction(_viewCollectionAction);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(_addDocumentAction);
        BaseClass::_contextMenu->addAction(_updateDocumentAction);
        BaseClass::_contextMenu->addAction(_removeDocumentAction);
        BaseClass::_contextMenu->addAction(_removeAllDocumentsAction);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(_renameCollectionAction);
        BaseClass::_contextMenu->addAction(_duplicateCollectionAction);
        BaseClass::_contextMenu->addAction(_copyCollectionToDiffrentServerAction);
        BaseClass::_contextMenu->addAction(_dropCollectionAction);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(_collectionStatsAction);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(_shardVersionAction);
        BaseClass::_contextMenu->addAction(_shardDistributionAction);

        AppRegistry::instance().bus()->subscribe(_databaseItem, LoadCollectionIndexesResponse::Type, this);
        AppRegistry::instance().bus()->subscribe(_databaseItem, DeleteCollectionIndexResponse::Type, this);
        AppRegistry::instance().bus()->subscribe(this, CollectionIndexesLoadingEvent::Type, this);
        
        setText(0, QtUtils::toQString(_collection->name()));
        setIcon(0, GuiRegistry::instance().collectionIcon());

        _indexDir = new ExplorerCollectionDirIndexesTreeItem(this);
        addChild(_indexDir);
        setToolTip(0, buildToolTip(collection));

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        
        retranslateUI();
    }
    
    void ExplorerCollectionTreeItem::retranslateUI()
    {
        _addDocumentAction->setText(tr("Insert Document..."));
        _updateDocumentAction->setText(tr("Update Documents..."));
        _removeDocumentAction->setText(tr("Remove Documents..."));
        _removeAllDocumentsAction->setText(tr("Remove All Documents..."));
        _collectionStatsAction->setText(tr("Statistics"));
        _storageSizeAction->setText(tr("Storage Size"));
        _totalIndexSizeAction->setText(tr("Total Index Size"));
        _totalSizeAction->setText(tr("Total Size"));
        _shardVersionAction->setText(tr("Shard Version"));
        _shardDistributionAction->setText(tr("Shard Distribution"));
        _dropCollectionAction->setText(tr("Drop Collection..."));
        _renameCollectionAction->setText(tr("Rename Collection..."));
        _duplicateCollectionAction->setText(tr("Duplicate Collection..."));
        _copyCollectionToDiffrentServerAction->setText(tr("Copy Collection to Database..."));
        _viewCollectionAction->setText(tr("View Documents"));
    }

    void ExplorerCollectionTreeItem::handle(LoadCollectionIndexesResponse *event)
    {
        QtUtils::clearChildItems(_indexDir);
        const std::vector<EnsureIndexInfo> &indexes = event->indexes();
        for (std::vector<EnsureIndexInfo>::const_iterator it = indexes.begin(); it!=indexes.end(); ++it) {
            _indexDir->addChild(new ExplorerCollectionIndexesTreeItem(_indexDir,*it));
        }
        _indexDir->setText(0, detail::buildName(ExplorerCollectionDirIndexesTreeItem::labelText,_indexDir->childCount()));
    }

    void ExplorerCollectionTreeItem::handle(DeleteCollectionIndexResponse *event)
    {
        if (!event->index().empty()) {
            int itemCount = _indexDir->childCount();
            QString eventIndex = QtUtils::toQString(event->index());
            for (int i = 0; i < itemCount; ++i) {
                QTreeWidgetItem *item = _indexDir->child(i);
                if (item->text(0) == eventIndex) {
                    removeChild(item);
                    delete item;
                    break;
                }
            }
        }
        _indexDir->setText(0, detail::buildName(ExplorerCollectionDirIndexesTreeItem::labelText, _indexDir->childCount()));
    }

    void ExplorerCollectionTreeItem::handle(CollectionIndexesLoadingEvent *event)
    {
        _indexDir->setText(0, detail::buildName(ExplorerCollectionDirIndexesTreeItem::labelText, -1));
    }

    void ExplorerCollectionTreeItem::expand()
    {
         AppRegistry::instance().bus()->publish(new CollectionIndexesLoadingEvent(this));
         if (_databaseItem) {
             _databaseItem->expandColection(this);
         }
    }

    void ExplorerCollectionTreeItem::dropIndex(const QTreeWidgetItem * const ind)
    {
        if (!_databaseItem)
            return;

        _databaseItem->dropIndexFromCollection(this, QtUtils::toStdString(ind->text(0)));
    }

    QString ExplorerCollectionTreeItem::buildToolTip(MongoCollection *collection)
    {
        char buff[2048]={0};
        sprintf(buff,tooltipTemplate,collection->name().c_str(),collection->info().count(),collection->sizeString().c_str());
        return buff;
    }

    void ExplorerCollectionTreeItem::ui_addDocument()
    {
        MongoDatabase *database = _collection->database();
        MongoServer *server = database->server();
        ConnectionSettings *settings = server->connectionRecord();

        DocumentTextEditor editor(CollectionInfo(settings->getFullAddress(), database->name(), _collection->name()), "{\n    \n}");

        editor.setCursorPosition(1, 4);
        editor.setWindowTitle(tr("Insert Document"));
        int result = editor.exec();

        treeWidget()->activateWindow();

        if (result == QDialog::Accepted) {
            server->insertDocuments(editor.bsonObj(), MongoNamespace(database->name(), _collection->name()) );
        }
    }

    void ExplorerCollectionTreeItem::ui_removeDocument()
    {
        openCurrentCollectionShell(
            "remove({ '' : '' });"
            , false, CursorPosition(0, -10));
    }

    void ExplorerCollectionTreeItem::ui_removeAllDocuments()
    {
        MongoDatabase *database = _collection->database();
        // Ask user
        int answer = QMessageBox::question(treeWidget(),
            tr("Remove All Documents"),
            tr("Remove all documents from <b>%1</b> collection?").arg(QtUtils::toQString(_collection->name())),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        if (answer == QMessageBox::Yes) {
            MongoServer *server = database->server();
            mongo::BSONObjBuilder builder;
            mongo::BSONObj bsonQuery = builder.obj();
            mongo::Query query(bsonQuery);
            server->removeDocuments(query, MongoNamespace(database->name(), _collection->name()), false);
        }
    }

    void ExplorerCollectionTreeItem::ui_updateDocument()
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
            "        \"multi\" : false,  // " + tr("update only one document") + " \n"
            "        \"upsert\" : false  // " + tr("insert a new document, if no existing document match the query") + " \n"
            "    }\n"
            ");", false);
    }

    void ExplorerCollectionTreeItem::ui_collectionStatistics()
    {
        openCurrentCollectionShell("stats()");
    }

    void ExplorerCollectionTreeItem::ui_dropCollection()
    {
        // Ask user
        int answer = utils::questionDialog(treeWidget(),tr("Drop"),tr("collection"),QtUtils::toQString(_collection->name()));

        if (answer == QMessageBox::Yes) {
            MongoDatabase *database = _collection->database();
            database->dropCollection(_collection->name());
            database->loadCollections();
        }
    }

    void ExplorerCollectionTreeItem::ui_duplicateCollection()
    {
        MongoDatabase *database = _collection->database();
        MongoServer *server = database->server();
        ConnectionSettings *settings = server->connectionRecord();

        CreateDatabaseDialog dlg(QtUtils::toQString(settings->getFullAddress()),
            QtUtils::toQString(database->name()),
            QtUtils::toQString(_collection->name()), treeWidget());
        dlg.setWindowTitle(tr("Duplicate Collection"));
        dlg.setOkButtonText(tr("&Duplicate"));
        dlg.setInputLabelText(tr("New Collection Name:"));
        dlg.setInputText(QtUtils::toQString(_collection->name() + "_copy"));
        int result = dlg.exec();

        if (result == QDialog::Accepted) {
            database->duplicateCollection(_collection->name(), QtUtils::toStdString(dlg.databaseName()));

            // refresh list of collections
            database->loadCollections();
        }
    }

    void ExplorerCollectionTreeItem::ui_copyToCollectionToDiffrentServer()
    {
        MongoDatabase *databaseFrom = _collection->database();
        MongoServer *server = databaseFrom->server();
        ConnectionSettings *settings = server->connectionRecord();

        CopyCollection dlg(QtUtils::toQString(settings->getFullAddress()), QtUtils::toQString(databaseFrom->name()), QtUtils::toQString(_collection->name()) );
        int result = dlg.exec();

        if (result == QDialog::Accepted) {
            MongoDatabase *databaseTo = dlg.selectedDatabase();
            databaseTo->copyCollection(server, databaseFrom->name(), _collection->name());
            databaseTo->loadCollections();
        }
    }

    void ExplorerCollectionTreeItem::ui_renameCollection()
    {
        MongoDatabase *database = _collection->database();
        MongoServer *server = database->server();
        ConnectionSettings *settings = server->connectionRecord();

        CreateDatabaseDialog dlg(QtUtils::toQString(settings->getFullAddress()),
            QtUtils::toQString(database->name()),
            QtUtils::toQString(_collection->name()), treeWidget());
        dlg.setWindowTitle(tr("Rename Collection"));
        dlg.setOkButtonText(tr("&Rename"));
        dlg.setInputLabelText(tr("New Collection Name:"));
        dlg.setInputText(QtUtils::toQString(_collection->name()));
        int result = dlg.exec();

        if (result == QDialog::Accepted) {
            database->renameCollection(_collection->name(), QtUtils::toStdString(dlg.databaseName()));
            // refresh list of collections
            database->loadCollections();
        }
    }

    void ExplorerCollectionTreeItem::ui_viewCollection()
    {
        openCurrentCollectionShell("find()");
    }

    void ExplorerCollectionTreeItem::ui_storageSize()
    {
        openCurrentCollectionShell("storageSize()");
    }

    void ExplorerCollectionTreeItem::ui_totalIndexSize()
    {
        openCurrentCollectionShell("totalIndexSize()");
    }

    void ExplorerCollectionTreeItem::ui_totalSize()
    {
        openCurrentCollectionShell("totalSize()");
    }

    void ExplorerCollectionTreeItem::ui_shardVersion()
    {
        openCurrentCollectionShell("getShardVersion()");
    }

    void ExplorerCollectionTreeItem::ui_shardDistribution()
    {
        openCurrentCollectionShell("getShardDistribution()");
    }

    void ExplorerCollectionTreeItem::openCurrentCollectionShell(const QString &script, bool execute,const CursorPosition &cursor)
    {
        QString query = detail::buildCollectionQuery(_collection->name(), script);
        AppRegistry::instance().app()->openShell(_collection->database(), query, execute, QtUtils::toQString(_collection->name()), cursor);
    }
}
