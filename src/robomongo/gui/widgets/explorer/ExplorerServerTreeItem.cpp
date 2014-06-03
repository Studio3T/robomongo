#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"
#include "robomongo/gui/GuiRegistry.h"


namespace
{
     void openCurrentServerShell(Robomongo::MongoServer *const server,const QString &script, bool execute = true, const Robomongo::CursorPosition &cursor = Robomongo::CursorPosition())
     {
          Robomongo::AppRegistry::instance().app()->openShell(server, script, std::string(), execute, Robomongo::QtUtils::toQString(server->connectionRecord()->getReadableName()), cursor);
     }
}

namespace Robomongo
{
    ExplorerServerTreeItem::ExplorerServerTreeItem(QTreeWidget *view,MongoServer *const server) : BaseClass(view),
        _server(server),
        _bus(AppRegistry::instance().bus())
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

        setText(0, buildServerName());
        setIcon(0, GuiRegistry::instance().serverIcon());
        setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerServerTreeItem::expand()
    {
        _server->loadDatabases();
    }

    void ExplorerServerTreeItem::databaseRefreshed(const QList<MongoDatabase *> &dbs)
    {
        int count = dbs.count();
        setText(0, buildServerName(&count));

        // Remove child items
        QtUtils::clearChildItems(this);

        // Add 'System' folder
        QIcon folderIcon = GuiRegistry::instance().folderIcon();
        ExplorerTreeItem *systemFolder = new ExplorerTreeItem(this);
        systemFolder->setIcon(0, folderIcon);
        systemFolder->setText(0, "System");
        addChild(systemFolder);

        for (int i = 0; i < dbs.size(); i++)
        {
            MongoDatabase *database = dbs.at(i);

            if (database->isSystem()) {
                ExplorerDatabaseTreeItem *dbItem = new ExplorerDatabaseTreeItem(systemFolder,database);
                systemFolder->addChild(dbItem);
                continue;
            }

            ExplorerDatabaseTreeItem *dbItem = new ExplorerDatabaseTreeItem(this,database);
            addChild(dbItem);
        }

        // Show 'System' folder only if it has items
        systemFolder->setHidden(systemFolder->childCount() == 0);
    }

    void ExplorerServerTreeItem::handle(DatabaseListLoadedEvent *event)
    {
        databaseRefreshed(event->list);
    }

    void ExplorerServerTreeItem::handle(MongoServerLoadingDatabasesEvent *event)
    {
        int count = -1;
        setText(0, buildServerName(&count));
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
        openCurrentServerShell(_server,"db.hostInfo()");
    }

    void ExplorerServerTreeItem::ui_serverStatus()
    {
        openCurrentServerShell(_server,"db.serverStatus()");
    }

    void ExplorerServerTreeItem::ui_serverVersion()
    {
        openCurrentServerShell(_server,"db.version()");
    }

    void ExplorerServerTreeItem::ui_showLog()
    {
        openCurrentServerShell(_server,"show log");
    }

    void ExplorerServerTreeItem::ui_openShell()
    {
        openCurrentServerShell(_server,"");
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
        CreateDatabaseDialog dlg(QtUtils::toQString(_server->connectionRecord()->getFullAddress()), QString(), QString(), treeWidget());
        dlg.setOkButtonText("&Create");
        dlg.setInputLabelText("Database Name:");
        int result = dlg.exec();
        if (result == QDialog::Accepted) {
            _server->createDatabase(QtUtils::toStdStringSafe(dlg.databaseName()));

            // refresh list of databases
            expand();
        }
    } 
}
