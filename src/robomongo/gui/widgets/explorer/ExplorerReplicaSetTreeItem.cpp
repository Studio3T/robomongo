#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetTreeItem.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QString>

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
    // todo : modify or move to common header
     void openCurrentServerShell(Robomongo::MongoServer *const server, const QString &script, bool execute = true, 
                                 const Robomongo::CursorPosition &cursor = Robomongo::CursorPosition())
     {
          Robomongo::AppRegistry::instance().app()->openShell(server, script, std::string(), execute, 
              Robomongo::QtUtils::toQString(server->connectionRecord()->getReadableName()), cursor);
     }
}

namespace Robomongo
{
    ExplorerReplicaSetTreeItem::ExplorerReplicaSetTreeItem(QTreeWidgetItem *parent, MongoServer *const server, 
        const std::string& replicaMemberIpPort)
        : BaseClass(parent),
        _server(server),
        _bus(AppRegistry::instance().bus())
    { 
        QAction *openShellAction = new QAction("Open Shell", this);
        openShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
        VERIFY(connect(openShellAction, SIGNAL(triggered()), SLOT(ui_openShell())));

        QAction *refreshServer = new QAction("Refresh", this);
        VERIFY(connect(refreshServer, SIGNAL(triggered()), SLOT(ui_refreshServer())));

        QAction *serverStatus = new QAction("Server Status", this);
        VERIFY(connect(serverStatus, SIGNAL(triggered()), SLOT(ui_serverStatus())));

        QAction *serverVersion = new QAction("MongoDB Version", this);
        VERIFY(connect(serverVersion, SIGNAL(triggered()), SLOT(ui_serverVersion())));

        QAction *serverHostInfo = new QAction("Host Info", this);
        VERIFY(connect(serverHostInfo, SIGNAL(triggered()), SLOT(ui_serverHostInfo())));        

        QAction *showLog = new QAction("Show Log", this);
        VERIFY(connect(showLog, SIGNAL(triggered()), SLOT(ui_showLog()))); 

        BaseClass::_contextMenu->addAction(openShellAction);
        BaseClass::_contextMenu->addAction(refreshServer);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(serverStatus);
        BaseClass::_contextMenu->addAction(serverHostInfo);
        BaseClass::_contextMenu->addAction(serverVersion);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(showLog);

        //_bus->subscribe(this, DatabaseListLoadedEvent::Type, _server);
        //_bus->subscribe(this, MongoServerLoadingDatabasesEvent::Type, _server);

        setText(0, QString::fromStdString(replicaMemberIpPort));
        setIcon(0, GuiRegistry::instance().serverIcon());
        setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerReplicaSetTreeItem::ui_serverHostInfo()
    {
        openCurrentServerShell(_server, "db.hostInfo()");
    }

    void ExplorerReplicaSetTreeItem::ui_serverStatus()
    {
        openCurrentServerShell(_server, "db.serverStatus()");
    }

    void ExplorerReplicaSetTreeItem::ui_serverVersion()
    {
        openCurrentServerShell(_server, "db.version()");
    }

    void ExplorerReplicaSetTreeItem::ui_showLog()
    {
        openCurrentServerShell(_server, "show log");
    }

    void ExplorerReplicaSetTreeItem::ui_openShell()
    {
        openCurrentServerShell(_server, "");
    }

    void ExplorerReplicaSetTreeItem::ui_refreshServer()
    {
        
    }

}
