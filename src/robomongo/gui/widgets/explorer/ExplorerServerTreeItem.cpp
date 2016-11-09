#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetFolderItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetTreeItem.h"
#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"
#include "robomongo/gui/GuiRegistry.h"


namespace
{
     void openCurrentServerShell(Robomongo::MongoServer *const server, const QString &script, bool execute = true, 
                                 const Robomongo::CursorPosition &cursor = Robomongo::CursorPosition())
     {
         QString const connName = Robomongo::QtUtils::toQString(server->connectionRecord()->getReadableName());
         Robomongo::AppRegistry::instance().app()->openShell(server, script, std::string(), execute, connName, cursor);
     }
}

namespace Robomongo
{
    ExplorerServerTreeItem::ExplorerServerTreeItem(QTreeWidget *view, MongoServer *const server) : 
        BaseClass(view), _server(server), _bus(AppRegistry::instance().bus()), _replicaSetFolder(nullptr),
        _primaryWasUnreachable(false), _systemFolder(nullptr)
    { 
        QAction *openShellAction = new QAction("Open Shell", this);
        openShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
        VERIFY(connect(openShellAction, SIGNAL(triggered()), SLOT(ui_openShell())));

        QAction *refreshServer = new QAction("Refresh", this);
        VERIFY(connect(refreshServer, SIGNAL(triggered()), SLOT(ui_refreshServer())));

        QAction *createDatabase = new QAction("Create Database", this);
        VERIFY(connect(createDatabase, SIGNAL(triggered()), SLOT(ui_createDatabase())));

        QAction *serverStatus = new QAction("Server Status", this);
        VERIFY(connect(serverStatus, SIGNAL(triggered()), SLOT(ui_serverStatus())));

        QAction *serverVersion = new QAction("MongoDB Version", this);
        VERIFY(connect(serverVersion, SIGNAL(triggered()), SLOT(ui_serverVersion())));

        QAction *serverHostInfo = new QAction("Host Info", this);
        VERIFY(connect(serverHostInfo, SIGNAL(triggered()), SLOT(ui_serverHostInfo())));        

        QAction *showLog = new QAction("Show Log", this);
        VERIFY(connect(showLog, SIGNAL(triggered()), SLOT(ui_showLog()))); 

        QAction *disconnectAction = new QAction("Disconnect", this);
        disconnectAction->setIconText("Disconnect");
        VERIFY(connect(disconnectAction, SIGNAL(triggered()), SLOT(ui_disconnectServer())));

        BaseClass::_contextMenu->addAction(openShellAction);
        BaseClass::_contextMenu->addAction(refreshServer);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(createDatabase);
        BaseClass::_contextMenu->addAction(serverStatus);
        BaseClass::_contextMenu->addAction(serverHostInfo);
        BaseClass::_contextMenu->addAction(serverVersion);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(showLog);
        BaseClass::_contextMenu->addAction(disconnectAction);

        _bus->subscribe(this, DatabaseListLoadedEvent::Type, _server);
        _bus->subscribe(this, MongoServerLoadingDatabasesEvent::Type, _server);
        _bus->subscribe(this, ReplicaSetUpdated::Type, _server);

        setText(0, buildServerName());
        if (_server->connectionRecord()->isReplicaSet())
        {
            setIcon(0, GuiRegistry::instance().replicaSetIcon());
        }
        else {
            setIcon(0, GuiRegistry::instance().serverIcon());
        }
        setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerServerTreeItem::expand()
    {
        _server->tryRefresh();
        //buildReplicaSetFolder();  // todo
        _server->loadDatabases();
    }

    void ExplorerServerTreeItem::databaseRefreshed(const QList<MongoDatabase *> &dbs)
    {
        int count = dbs.count();
        setText(0, buildServerName(&count));

        // Delete system folder and database items
        QtUtils::clearChildItems(this); // todo: clear database items

        // todo: 
        if (_server->connectionRecord()->isReplicaSet()) {
            _replicaSetFolder = nullptr;    // todo
            buildReplicaSetFolder();
        }

        // Add 'System' folder
        QIcon folderIcon = GuiRegistry::instance().folderIcon();
        _systemFolder = new ExplorerTreeItem(this);
        _systemFolder->setIcon(0, folderIcon);
        _systemFolder->setText(0, "System");
        addChild(_systemFolder);

        for (int i = 0; i < dbs.size(); i++)
        {
            MongoDatabase *database = dbs.at(i);

            if (database->isSystem()) {
                auto dbItem = new ExplorerDatabaseTreeItem(_systemFolder, database);
                _systemFolder->addChild(dbItem);
                continue;
            }

            auto dbItem = new ExplorerDatabaseTreeItem(this, database);
            addChild(dbItem);
        }

        // Show 'System' folder only if it has items
        _systemFolder->setHidden(_systemFolder->childCount() == 0);
    }

    void ExplorerServerTreeItem::handle(DatabaseListLoadedEvent *event)
    {
        if (event->isError()) {
            setText(0, buildServerName(0));     // Restore normal name (without "...")
            
            // Todo
            if (!_server->connectionRecord()->isReplicaSet()) 
                setExpanded(false);                 // Collapse

            // We should clear all child items, but we are not
            // doing this, because of incorrect "senders" for messages
            // that are going to Worker thread. Yes, some guy specified UI objects
            // as senders. If we delete these UI objects, response will crash the app
            // (because QObject::thread() is used to get thread of the QObject)
            // QtUtils::clearChildItems(this);

            std::stringstream ss;
            ss << "Cannot load list of databases.\n\nError:\n"
                << event->error().errorMessage();

            QMessageBox::information(NULL, "Error", QtUtils::toQString(ss.str()));
            return;
        }

        databaseRefreshed(event->list);
    }

    void ExplorerServerTreeItem::handle(MongoServerLoadingDatabasesEvent *event)
    {
        int count = -1;
        setText(0, buildServerName(&count));
    }

    void ExplorerServerTreeItem::handle(ReplicaSetUpdated *event)
    {
        buildReplicaSetFolder();

        if (event->isError())  // ---Primary is unreachable
        { 
            _primaryWasUnreachable = true;

            // For system folder and database items - delete children then set disable
            QtUtils::clearChildItems(_systemFolder);
            _systemFolder->setDisabled(true);
            for ( int i = 0 ; i < childCount(); ++i)  {
                auto dbItem = dynamic_cast<ExplorerDatabaseTreeItem*>(child(i));
                if (dbItem) {
                    QtUtils::clearChildItems(dbItem);
                    dbItem->setDisabled(true);
                }
            }

            // Show error dialog
            std::string const errorStr = "Set's primary is unreachable.\n\nError:\n" +
                                          event->error().errorMessage();
            QMessageBox::critical(nullptr, "Error", QString::fromStdString(errorStr));
            return;
        }

        // --- Primary is reachable
        // If primary was unreachable previously, rebuild db items 
        if (_primaryWasUnreachable) {
            //_server->tryRefresh();
            _server->loadDatabases();
        }
        _primaryWasUnreachable = false;
    }

    QString ExplorerServerTreeItem::buildServerName(int *count /* = NULL */)
    {
        QString name = QtUtils::toQString(_server->connectionRecord()->getReadableName());

        if (!count)
            return name;

        if (*count == -1)
            return name + " ...";

        return QString("%1 (%2)").arg(name).arg(*count);
    }

    void ExplorerServerTreeItem::ui_serverHostInfo()
    {
        openCurrentServerShell(_server, "db.hostInfo()");
    }

    void ExplorerServerTreeItem::ui_serverStatus()
    {
        openCurrentServerShell(_server, "db.serverStatus()");
    }

    void ExplorerServerTreeItem::ui_serverVersion()
    {
        openCurrentServerShell(_server, "db.version()");
    }

    void ExplorerServerTreeItem::ui_showLog()
    {
        openCurrentServerShell(_server, "show log");
    }

    void ExplorerServerTreeItem::ui_openShell()
    {
        openCurrentServerShell(_server, "");
    }

    void ExplorerServerTreeItem::ui_disconnectServer()
    {
        QTreeWidget *view = dynamic_cast<QTreeWidget *>(BaseClass::treeWidget());
        if (!view)
            return;

        int index = view->indexOfTopLevelItem(this);
        if (index == -1)
            return;

        QTreeWidgetItem *removedItem = view->takeTopLevelItem(index);
        if (!removedItem)
            return;

        AppRegistry::instance().app()->closeServer(_server);
        delete removedItem;
    }

    void ExplorerServerTreeItem::ui_refreshServer()
    {
        expand();
    }

    void ExplorerServerTreeItem::ui_createDatabase()
    {
        CreateDatabaseDialog dlg(QString::fromStdString(_server->connectionRecord()->getFullAddress()), 
                                 QString(), QString(), treeWidget());
        dlg.setOkButtonText("&Create");
        dlg.setInputLabelText("Database Name:");
        int result = dlg.exec();
        if (result == QDialog::Accepted) {
            _server->createDatabase(QtUtils::toStdStringSafe(dlg.databaseName()));

            // refresh list of databases
            expand();
        }
    } 

    void ExplorerServerTreeItem::buildReplicaSetFolder()
    {
        // todo: use event object instead if using _server for fresh replica set information

        if (_replicaSetFolder) {
            if (_replicaSetFolder->childCount()>0)
                QtUtils::clearChildItems(_replicaSetFolder);

            _replicaSetFolder->updateText();
        }
        else {
            _replicaSetFolder = new ExplorerReplicaSetFolderItem(this, _server);
            addChild(_replicaSetFolder);
        }

        // Add replica members
        bool isPrimary;
        for (auto const& memberAndHealth : _server->replicaSetInfo()->membersAndHealths)
        {
            isPrimary = (_server->replicaSetInfo()->primary.toString() == memberAndHealth.first);
            auto hostAndPort = mongo::HostAndPort(mongo::StringData(memberAndHealth.first));
            _replicaSetFolder->addChild(new ExplorerReplicaSetTreeItem(_replicaSetFolder, _server, hostAndPort, 
                                        isPrimary, memberAndHealth.second));
        }
        _replicaSetFolder->setExpanded(true);
    }
}
