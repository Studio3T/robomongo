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
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/events/MongoEvents.hpp"

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
    const QString ExplorerCollectionDirIndexesTreeItem::labelText = "Indexes";

    ExplorerCollectionDirIndexesTreeItem::ExplorerCollectionDirIndexesTreeItem(QTreeWidgetItem *parent)
        :BaseClass(parent)
    {
        QAction *addIndex = new QAction("Add Index...", this);
        VERIFY(connect(addIndex, SIGNAL(triggered()), SLOT(ui_addIndex())));

        QAction *addIndexGui = new QAction("Add Index...", this);
        VERIFY(connect(addIndexGui, SIGNAL(triggered()), SLOT(ui_addIndexGui())));

        QAction *dropIndex = new QAction("Drop Index...", this);
        VERIFY(connect(dropIndex, SIGNAL(triggered()), SLOT(ui_dropIndex())));

        QAction *reIndex = new QAction("Rebuild Indexes...", this);
        VERIFY(connect(reIndex, SIGNAL(triggered()), SLOT(ui_reIndex())));

        QAction *viewIndex = new QAction("View Indexes", this);
        VERIFY(connect(viewIndex, SIGNAL(triggered()), SLOT(ui_viewIndex())));

        QAction *refreshIndex = new QAction("Refresh", this);
        VERIFY(connect(refreshIndex, SIGNAL(triggered()), SLOT(ui_refreshIndex())));

        BaseClass::_contextMenu->addAction(viewIndex);
        //BaseClass::_contextMenu->addAction(addIndex);
        BaseClass::_contextMenu->addAction(addIndexGui);
        //BaseClass::_contextMenu->addAction(dropIndex);
        BaseClass::_contextMenu->addAction(reIndex);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(refreshIndex);      

        setText(0, labelText);
        setIcon(0, Robomongo::GuiRegistry::instance().folderIcon());

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
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
                "// { unique : true }   - A unique index causes MongoDB to reject all documents that contain a duplicate value for the indexed field. \n"
                "// { sparse : true }   - Sparse indexes only contain entries for documents that have the indexed field. \n"
                "// { dropDups : true } - Sparse indexes only contain entries for documents that have the indexed field. \n"
                , false);
        }
    }

    void ExplorerCollectionDirIndexesTreeItem::ui_addIndexGui()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *const>(parent());
        if (!par)
            return;
        EnsureIndex fakeInfo(par->collection()->info(),"");
        EditIndexDialog dlg(fakeInfo , QtUtils::toQString(par->databaseItem()->database()->name()),QtUtils::toQString(par->databaseItem()->database()->server()->connectionRecord()->getFullAddress()), treeWidget());
        int result = dlg.exec();
        if (result != QDialog::Accepted)
            return;

        par->collection()->createIndex(fakeInfo, dlg.info());
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

    ExplorerCollectionIndexesTreeItem::ExplorerCollectionIndexesTreeItem(ExplorerCollectionDirIndexesTreeItem *parent,const EnsureIndex &info)
        : BaseClass(parent),_info(info)
    {
        QAction *deleteIndex = new QAction("Drop Index...", this);
        connect(deleteIndex, SIGNAL(triggered()), SLOT(ui_dropIndex()));
        QAction *editIndex = new QAction("Edit Index...", this);
        connect(editIndex, SIGNAL(triggered()), SLOT(ui_edit()));

        BaseClass::_contextMenu->addAction(editIndex);
        BaseClass::_contextMenu->addAction(deleteIndex);

        setText(0, QtUtils::toQString(_info._name));
        setIcon(0, Robomongo::GuiRegistry::instance().indexIcon());
    }

    void ExplorerCollectionIndexesTreeItem::ui_dropIndex()
    {
        // Ask user
        int answer = utils::questionDialog(treeWidget(),"Drop","Index",text(0));

        if (answer != QMessageBox::Yes)
            return;

        ExplorerCollectionDirIndexesTreeItem *par = dynamic_cast<ExplorerCollectionDirIndexesTreeItem *>(parent());
        if (!par)
            return;
        
        ExplorerCollectionTreeItem *grandParent = dynamic_cast<ExplorerCollectionTreeItem *>(par->parent());
        if (!grandParent)
            return;

        grandParent->collection()->dropIndex(QtUtils::toStdString(text(0)));
    }

    void ExplorerCollectionIndexesTreeItem::ui_edit()
    {
        ExplorerCollectionDirIndexesTreeItem *par = dynamic_cast<ExplorerCollectionDirIndexesTreeItem *>(parent());           
        if (par) {
            ExplorerCollectionTreeItem *grPar = dynamic_cast<ExplorerCollectionTreeItem *const>(par->parent());
            if (!grPar)
                return;

            EditIndexDialog dlg(_info, QtUtils::toQString(grPar->databaseItem()->database()->name()), QtUtils::toQString(grPar->databaseItem()->database()->server()->connectionRecord()->getFullAddress()), treeWidget());
            int result = dlg.exec();
            if (result != QDialog::Accepted)
                return;

            grPar->collection()->createIndex(_info, dlg.info());
        }
    }

    ExplorerCollectionTreeItem::ExplorerCollectionTreeItem(MongoServer *server, QTreeWidgetItem *parent, ExplorerDatabaseTreeItem *databaseItem, MongoCollection *collection) :
        BaseClass(parent), _server(server), _collection(collection), _databaseItem(databaseItem)
    {
        VERIFY(connect(_collection, SIGNAL(startedIndexListLoad(const EventsInfo::LoadCollectionIndexesRequestInfo &)), this, SLOT(startIndexListLoad(const EventsInfo::LoadCollectionIndexesRequestInfo &)), Qt::DirectConnection));
        VERIFY(connect(_collection, SIGNAL(finishedIndexListLoad(const EventsInfo::LoadCollectionIndexesResponceInfo &)), this, SLOT(finishIndexListLoad(const EventsInfo::LoadCollectionIndexesResponceInfo &)), Qt::DirectConnection));

        QAction *addDocument = new QAction("Insert Document...", this);
        VERIFY(connect(addDocument, SIGNAL(triggered()), SLOT(ui_addDocument())));

        QAction *updateDocument = new QAction("Update Documents...", this);
        VERIFY(connect(updateDocument, SIGNAL(triggered()), SLOT(ui_updateDocument())));
        QAction *removeDocument = new QAction("Remove Documents...", this);
        VERIFY(connect(removeDocument, SIGNAL(triggered()), SLOT(ui_removeDocument())));

        QAction *removeAllDocuments = new QAction("Remove All Documents...", this);
        VERIFY(connect(removeAllDocuments, SIGNAL(triggered()), SLOT(ui_removeAllDocuments())));

        QAction *collectionStats = new QAction("Statistics", this);
        VERIFY(connect(collectionStats, SIGNAL(triggered()), SLOT(ui_collectionStatistics())));

        QAction *storageSize = new QAction("Storage Size", this);
        VERIFY(connect(storageSize, SIGNAL(triggered()), SLOT(ui_storageSize())));

        QAction *totalIndexSize = new QAction("Total Index Size", this);
        VERIFY(connect(totalIndexSize, SIGNAL(triggered()), SLOT(ui_totalIndexSize())));

        QAction *totalSize = new QAction("Total Size", this);
        VERIFY(connect(totalSize, SIGNAL(triggered()), SLOT(ui_totalSize())));
        QAction *shardVersion = new QAction("Shard Version", this);
        VERIFY(connect(shardVersion, SIGNAL(triggered()), SLOT(ui_shardVersion())));

        QAction *shardDistribution = new QAction("Shard Distribution", this);
        VERIFY(connect(shardDistribution, SIGNAL(triggered()), SLOT(ui_shardDistribution())));

        QAction *dropCollection = new QAction("Drop Collection...", this);
        VERIFY(connect(dropCollection, SIGNAL(triggered()), SLOT(ui_dropCollection())));

        QAction *renameCollection = new QAction("Rename Collection...", this);
        VERIFY(connect(renameCollection, SIGNAL(triggered()), SLOT(ui_renameCollection())));
        QAction *duplicateCollection = new QAction("Duplicate Collection...", this);
        VERIFY(connect(duplicateCollection, SIGNAL(triggered()), SLOT(ui_duplicateCollection())));
        QAction *copyCollectionToDiffrentServer = new QAction("Copy Collection to Database...", this);
        VERIFY(connect(copyCollectionToDiffrentServer, SIGNAL(triggered()), SLOT(ui_copyToCollectionToDiffrentServer())));

        QAction *viewCollection = new QAction("View Documents", this);
        VERIFY(connect(viewCollection, SIGNAL(triggered()), SLOT(ui_viewCollection())));

        BaseClass::_contextMenu->addAction(viewCollection);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(addDocument);
        BaseClass::_contextMenu->addAction(updateDocument);
        BaseClass::_contextMenu->addAction(removeDocument);
        BaseClass::_contextMenu->addAction(removeAllDocuments);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(renameCollection);
        BaseClass::_contextMenu->addAction(duplicateCollection);
        BaseClass::_contextMenu->addAction(copyCollectionToDiffrentServer);
        BaseClass::_contextMenu->addAction(dropCollection);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(collectionStats);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(shardVersion);
        BaseClass::_contextMenu->addAction(shardDistribution);
        
        setText(0, QtUtils::toQString(_collection->name()));
        setIcon(0, GuiRegistry::instance().collectionIcon());

        _indexDir = new ExplorerCollectionDirIndexesTreeItem(this);
        addChild(_indexDir);
        setToolTip(0, buildToolTip(collection));

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerCollectionTreeItem::startIndexListLoad(const EventsInfo::LoadCollectionIndexesRequestInfo &inf)
    {
        _indexDir->setText(0, detail::buildName(ExplorerCollectionDirIndexesTreeItem::labelText, -1));
    }

    void ExplorerCollectionTreeItem::finishIndexListLoad(const EventsInfo::LoadCollectionIndexesResponceInfo &inf)
    {
        ErrorInfo er = inf.errorInfo();
        QtUtils::clearChildItems(_indexDir);
        if(!er.isError()){
            for (std::vector<EnsureIndex>::const_iterator it = inf._indexes.begin(); it!=inf._indexes.end(); ++it) {
                _indexDir->addChild(new ExplorerCollectionIndexesTreeItem(_indexDir,*it));
            }
            _indexDir->setText(0, detail::buildName(ExplorerCollectionDirIndexesTreeItem::labelText,_indexDir->childCount()));
        }  
    }

    void ExplorerCollectionTreeItem::expand()
    {         
         _collection->loadIndexes();
    }

    QString ExplorerCollectionTreeItem::buildToolTip(MongoCollection *collection)
    {
        char buff[2048]={0};
        sprintf(buff, tooltipTemplate, collection->name().c_str(), collection->info().count(), collection->sizeString().c_str());
        return buff;
    }

    void ExplorerCollectionTreeItem::ui_addDocument()
    {
        MongoDatabase *database = _collection->database();
        MongoServer *server = database->server();
        IConnectionSettingsBase *settings = server->connectionRecord();

        DocumentTextEditor editor(CollectionInfo(settings->getFullAddress(), database->name(), _collection->name()), "{\n    \n}");

        editor.setCursorPosition(1, 4);
        editor.setWindowTitle("Insert Document");
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
            "Remove All Documents",
            QString("Remove all documents from <b>%1</b> collection?").arg(QtUtils::toQString(_collection->name())),
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
            "        \"multi\" : false,  // update only one document \n"
            "        \"upsert\" : false  // insert a new document, if no existing document match the query \n"
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
        int answer = utils::questionDialog(treeWidget(),"Drop","collection",QtUtils::toQString(_collection->name()));

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
        IConnectionSettingsBase *settings = server->connectionRecord();

        CreateDatabaseDialog dlg(QtUtils::toQString(settings->getFullAddress()),
            QtUtils::toQString(database->name()),
            QtUtils::toQString(_collection->name()), treeWidget());
        dlg.setWindowTitle("Duplicate Collection");
        dlg.setOkButtonText("&Duplicate");
        dlg.setInputLabelText("New Collection Name:");
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
        IConnectionSettingsBase *settings = server->connectionRecord();

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
        IConnectionSettingsBase *settings = server->connectionRecord();

        CreateDatabaseDialog dlg(QtUtils::toQString(settings->getFullAddress()),
            QtUtils::toQString(database->name()),
            QtUtils::toQString(_collection->name()), treeWidget());
        dlg.setWindowTitle("Rename Collection");
        dlg.setOkButtonText("&Rename");
        dlg.setInputLabelText("New Collection Name:");
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
