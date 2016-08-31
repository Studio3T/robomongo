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
     void openCurrentServerShell(Robomongo::ConnectionSettings* connSettings, const QString &script)
     {
         auto const scriptStr = Robomongo::ScriptInfo(script, true);
         Robomongo::AppRegistry::instance().app()->openShell(connSettings, scriptStr);
     }
}

namespace Robomongo
{
    ExplorerReplicaSetTreeItem::ExplorerReplicaSetTreeItem(QTreeWidgetItem *parent, MongoServer *const server, 
        const mongo::HostAndPort& repMemberHostAndPort)
        : BaseClass(parent),
        _repMemberHostAndPort(repMemberHostAndPort),
        _server(server),
        _connSettings(server->connectionRecord()->clone()),
        _bus(AppRegistry::instance().bus())
    { 
        // Set connection settings of this replica member
        _connSettings->setServerHost(_repMemberHostAndPort.host());
        _connSettings->setServerPort(_repMemberHostAndPort.port());

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

        setText(0, QString::fromStdString(_repMemberHostAndPort.toString()));
        setIcon(0, GuiRegistry::instance().serverIcon());
        setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerReplicaSetTreeItem::ui_serverHostInfo()
    {
        openCurrentServerShell(_connSettings.get(), "db.hostInfo()");
    }

    void ExplorerReplicaSetTreeItem::ui_serverStatus()
    {
        openCurrentServerShell(_connSettings.get(), "db.serverStatus()");
    }

    void ExplorerReplicaSetTreeItem::ui_serverVersion()
    {
        openCurrentServerShell(_connSettings.get(), "db.version()");
    }

    void ExplorerReplicaSetTreeItem::ui_showLog()
    {
        openCurrentServerShell(_connSettings.get(), "show log");
    }

    void ExplorerReplicaSetTreeItem::ui_openShell()
    {
        openCurrentServerShell(_connSettings.get(), "");
    }

    void ExplorerReplicaSetTreeItem::ui_refreshServer()
    {
        // todo
    }

}
