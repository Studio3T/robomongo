#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"
#include "robomongo/core/AppRegistry.h"

namespace Robomongo
{
    ExplorerServerTreeItem::ExplorerServerTreeItem(MongoServer *server,QTreeWidget *view) : QObject(),BaseClass(view),
        _server(server),
        _bus(AppRegistry::instance().bus())
    {
       // QAction *refreshAction = new QAction("Refresh", this);
       // refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
       // connect(refreshAction, SIGNAL(triggered()), SLOT(ui_refreshServer()));   
        setExpanded(false);

        QAction *openShellAction = new QAction("Open Shell", this);
        openShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
        connect(openShellAction, SIGNAL(triggered()), SLOT(ui_openShell()));

        QAction *refreshServer = new QAction("Refresh", this);
        connect(refreshServer, SIGNAL(triggered()), SLOT(ui_refreshServer()));

        QAction *createDatabase = new QAction("Create Database", this);
        connect(createDatabase, SIGNAL(triggered()), SLOT(ui_createDatabase()));

        QAction *serverStatus = new QAction("Server Status", this);
        connect(serverStatus, SIGNAL(triggered()), SLOT(ui_serverStatus()));

        QAction *serverVersion = new QAction("MongoDB Version", this);
        connect(serverVersion, SIGNAL(triggered()), SLOT(ui_serverVersion()));

        QAction *serverHostInfo = new QAction("Host Info", this);
        connect(serverHostInfo, SIGNAL(triggered()), SLOT(ui_serverHostInfo()));        

        QAction *showLog = new QAction("Show Log", this);
        connect(showLog, SIGNAL(triggered()), SLOT(ui_showLog())); 

        QAction *disconnectAction = new QAction("Disconnect", this);
        disconnectAction->setIconText("Disconnect");
        connect(disconnectAction, SIGNAL(triggered()), SLOT(ui_disconnectServer()));

        BaseClass::_contextMenu.addAction(openShellAction);
        BaseClass::_contextMenu.addAction(refreshServer);
        BaseClass::_contextMenu.addSeparator();
        BaseClass::_contextMenu.addAction(createDatabase);
        BaseClass::_contextMenu.addAction(serverStatus);
        BaseClass::_contextMenu.addAction(serverHostInfo);
        BaseClass::_contextMenu.addAction(serverVersion);
        BaseClass::_contextMenu.addSeparator();
        BaseClass::_contextMenu.addAction(showLog);
        BaseClass::_contextMenu.addAction(disconnectAction);

        _bus->subscribe(this, DatabaseListLoadedEvent::Type, _server);
        _bus->subscribe(this, MongoServerLoadingDatabasesEvent::Type, _server);

        setText(0, buildServerName());
        setIcon(0, GuiRegistry::instance().serverIcon());
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    ExplorerServerTreeItem::~ExplorerServerTreeItem()
    {
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
        int itemCount = childCount();
        for (int i = 0; i < itemCount; ++i)
        {
            QTreeWidgetItem *p = child(0);
            removeChild(p);
            delete p;
        }

        // Add 'System' folder
        QIcon folderIcon = GuiRegistry::instance().folderIcon();
        QTreeWidgetItem *systemFolder = new QTreeWidgetItem();
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
        QString name = _server->connectionRecord()->getReadableName();

        if (!count)
            return name;

        if (*count == -1)
            return name + " ...";

        return QString("%1 (%2)").arg(name).arg(*count);
    }

    void ExplorerServerTreeItem::ui_serverHostInfo()
    {
        openCurrentServerShell("db.hostInfo()");
    }

    void ExplorerServerTreeItem::ui_serverStatus()
    {
        openCurrentServerShell("db.serverStatus()");
    }

    void ExplorerServerTreeItem::ui_serverVersion()
    {
        openCurrentServerShell("db.version()");
    }

    void ExplorerServerTreeItem::ui_showLog()
    {
        openCurrentServerShell("show log");
    }

    void ExplorerServerTreeItem::ui_openShell()
    {
        AppRegistry::instance().app()->openShell(_server, "");
    }

    void ExplorerServerTreeItem::ui_disconnectServer()
    {
        QTreeWidget * view = dynamic_cast<QTreeWidget *>(BaseClass::treeWidget());
        if(view){
            int index = view->indexOfTopLevelItem(this);
            if (index != -1) {
                QTreeWidgetItem *removedItem = view->takeTopLevelItem(index);
                if (removedItem) {
                    delete removedItem;
                    AppRegistry::instance().app()->closeServer(_server);
                }
            }
        }       
    }

    void ExplorerServerTreeItem::ui_refreshServer()
    {
        expand();
    }

    void ExplorerServerTreeItem::ui_createDatabase()
    {
        CreateDatabaseDialog dlg(this->server()->connectionRecord()->getFullAddress());
        dlg.setOkButtonText("&Create");
        dlg.setInputLabelText("Database Name:");
        int result = dlg.exec();
        if (result == QDialog::Accepted) {
            this->server()->createDatabase(dlg.databaseName());

            // refresh list of databases
            expand();
        }
    } 

    void ExplorerServerTreeItem::openCurrentServerShell(const QString &script, bool execute,const CursorPosition &cursor)
    {
        AppRegistry::instance().app()->openShell(_server, script, QString(), execute, _server->connectionRecord()->getReadableName(), cursor);
    }
}
