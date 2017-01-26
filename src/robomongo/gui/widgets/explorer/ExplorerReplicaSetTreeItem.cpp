#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetTreeItem.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QString>

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/dialogs/CreateDatabaseDialog.h"
#include "robomongo/gui/GuiRegistry.h"


namespace
{
     void openCurrentServerShell(Robomongo::MongoServer* server, Robomongo::ConnectionSettings* connSettings, 
                                 const QString &script)
     {
         auto const scriptStr = Robomongo::ScriptInfo(script, true);
         Robomongo::AppRegistry::instance().app()->openShell(server, connSettings, scriptStr);
     }
}

namespace Robomongo
{
    ExplorerReplicaSetTreeItem::ExplorerReplicaSetTreeItem(QTreeWidgetItem *parent, MongoServer *const server, 
        const mongo::HostAndPort& repMemberHostAndPort, const bool isPrimary, const bool isUp)
        : 
        BaseClass(parent),
        _repMemberHostAndPort(repMemberHostAndPort),
        _isPrimary(isPrimary),
        _isUp(isUp),
        _server(server),
        _connSettings(server->connectionRecord()->clone()),
        _bus(AppRegistry::instance().bus())
    { 
        // Set connection settings of this replica member
        _connSettings->setConnectionName(_repMemberHostAndPort.toString() + " [member of " + _connSettings->connectionName() + "]");
        _connSettings->setServerHost(_repMemberHostAndPort.host());
        _connSettings->setServerPort(_repMemberHostAndPort.port());
        _connSettings->setReplicaSet(false);  
        _connSettings->replicaSetSettings()->deleteAllMembers();

        // Add Actions
        auto openShellAction = new QAction("Open Shell", this);
#ifdef __APPLE__
        openShellAction->setIcon(GuiRegistry::instance().mongodbIconForMAC());
#else
        openShellAction->setIcon(GuiRegistry::instance().mongodbIcon());
#endif
        VERIFY(connect(openShellAction, SIGNAL(triggered()), SLOT(ui_openShell())));

        auto openDirectConnection = new QAction("Open Direct Connection", this);
        VERIFY(connect(openDirectConnection, SIGNAL(triggered()), SLOT(ui_openDirectConnection())));

        auto serverStatus = new QAction("Server Status", this);
        VERIFY(connect(serverStatus, SIGNAL(triggered()), SLOT(ui_serverStatus())));

        auto serverVersion = new QAction("MongoDB Version", this);
        VERIFY(connect(serverVersion, SIGNAL(triggered()), SLOT(ui_serverVersion())));

        auto serverHostInfo = new QAction("Host Info", this);
        VERIFY(connect(serverHostInfo, SIGNAL(triggered()), SLOT(ui_serverHostInfo())));        

        auto showLog = new QAction("Show Log", this);
        VERIFY(connect(showLog, SIGNAL(triggered()), SLOT(ui_showLog()))); 

        BaseClass::_contextMenu->addAction(openShellAction);
        BaseClass::_contextMenu->addAction(openDirectConnection);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(serverStatus);
        BaseClass::_contextMenu->addAction(serverHostInfo);
        BaseClass::_contextMenu->addAction(serverVersion);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(showLog);

        updateTextAndIcon(_isUp, _isPrimary);

        setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
    }

    void ExplorerReplicaSetTreeItem::updateTextAndIcon(bool isUp, bool isPrimary)
    {
        _isUp = isUp;
        _isPrimary = isPrimary;

        QString stateStr("[Unknown]");
        if (_isUp)
            stateStr = _isPrimary ? "[Primary]" : "[Secondary]";
        else
            stateStr = "[Not Reachable]";

        setDisabled(_isUp ? false : true);
        setText(0, QString::fromStdString(_repMemberHostAndPort.toString()) + " " + stateStr);
        setIcon(0, _isPrimary ? GuiRegistry::instance().serverPrimaryIcon()                               
                              : GuiRegistry::instance().serverSecondaryIcon());
    }

    void ExplorerReplicaSetTreeItem::ui_serverHostInfo()
    {
        openCurrentServerShell(_server, _connSettings.get(), "db.hostInfo()");
    }

    void ExplorerReplicaSetTreeItem::ui_serverStatus()
    {
        openCurrentServerShell(_server, _connSettings.get(), "db.serverStatus()");
    }

    void ExplorerReplicaSetTreeItem::ui_serverVersion()
    {
        openCurrentServerShell(_server, _connSettings.get(), "db.version()");
    }

    void ExplorerReplicaSetTreeItem::ui_showLog()
    {
        openCurrentServerShell(_server, _connSettings.get(), "show log");
    }

    void ExplorerReplicaSetTreeItem::ui_openShell()
    {
        openCurrentServerShell(_server, _connSettings.get(), "");
    }

    void ExplorerReplicaSetTreeItem::ui_openDirectConnection()
    {
        AppRegistry::instance().app()->openServer(_connSettings.get(), ConnectionPrimary);
    }
}
