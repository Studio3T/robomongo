#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetFolderItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/gui/GuiRegistry.h"

namespace
{
    void openCurrentServerShell(Robomongo::MongoServer *const server, const QString &script, bool execute = true, 
                                const Robomongo::CursorPosition &cursor = Robomongo::CursorPosition())
    {
        Robomongo::AppRegistry::instance().app()->openShell(server, script, std::string(), execute, 
            Robomongo::QtUtils::toQString(server->connectionRecord()->getReadableName()), cursor);
    }

    void openCurrentServerShell(Robomongo::MongoServer* server, Robomongo::ConnectionSettings* connSettings,
                                const QString &script)
    {
        Robomongo::AppRegistry::instance().app()->openShell(server, connSettings, Robomongo::ScriptInfo(script, true));
    }
}

namespace Robomongo
{

    ExplorerReplicaSetFolderItem::ExplorerReplicaSetFolderItem(ExplorerTreeItem *parent, MongoServer *const server) :
        BaseClass(parent), _server(server)
    {
        auto repSetStatus = new QAction("Status of Replica Set", this);
        VERIFY(connect(repSetStatus, SIGNAL(triggered()), SLOT(on_repSetStatus())));

        auto refresh = new QAction("Refresh", this);
        VERIFY(connect(refresh, SIGNAL(triggered()), SLOT(on_refresh())));

        BaseClass::_contextMenu->addAction(repSetStatus);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(refresh);

        AppRegistry::instance().bus()->subscribe(this, ReplicaSetFolderLoading::Type, _server);

        setIcon(0, GuiRegistry::instance().folderIcon());
        setText(0, "Replica Set (" + QString::number(_server->replicaSetInfo()->membersAndHealths.size()) + " nodes)");

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerReplicaSetFolderItem::updateText()
    {
        setText(0, "Replica Set (" + QString::number(_server->replicaSetInfo()->membersAndHealths.size()) + " nodes)");
    }

    void ExplorerReplicaSetFolderItem::disableSomeContextMenuActions()
    {
        if (BaseClass::_contextMenu->actions().size() < 1)
            return;

        // Find out if there is at least one reachable member  
        mongo::HostAndPort onlineMember;
        for (auto const& member : _server->replicaSetInfo()->membersAndHealths) {
            if (member.second) {
                onlineMember = mongo::HostAndPort(member.first);
                break;
            }
        }

        // Show menu item "Status of Replica Set" only if there is at least one reachable member
        // If there is no reachable member, disable it.
        BaseClass::_contextMenu->actions().at(0)->setDisabled(onlineMember.empty());
    }

    void ExplorerReplicaSetFolderItem::expand()
    {
        if (!_refreshFlag) {
            _refreshFlag = true;
            return;
        }

        _server->tryRefreshReplicaSetFolder(true);
    }

    void ExplorerReplicaSetFolderItem::on_repSetStatus()
    {
        if (!_server->replicaSetInfo()->primary.empty()) {
            openCurrentServerShell(_server, "rs.status()");
        }
        else // Primary is unreachable, try to find and run rs.status on a reachable secondary
        {
            mongo::HostAndPort onlineMember;
            for (auto const& member : _server->replicaSetInfo()->membersAndHealths) {
                if (member.second) {
                    onlineMember = mongo::HostAndPort(member.first);
                    break;
                }
            }

            if (onlineMember.empty()) {
                LOG_MSG("Unable to find a reachable replica set member to run rs.status().", 
                        mongo::logger::LogSeverity::Error());
                return;
            }

            auto connSetting = std::unique_ptr<ConnectionSettings>(_server->connectionRecord()->clone());
            // Set connection settings of this replica member
            connSetting->setConnectionName(onlineMember.toString() + 
                                           " [member of " + connSetting->connectionName() + "]");
            connSetting->setServerHost(onlineMember.host());
            connSetting->setServerPort(onlineMember.port());
            connSetting->setReplicaSet(false);
            connSetting->replicaSetSettings()->setMembers(std::vector<std::string>()); 

            openCurrentServerShell(_server, connSetting.get(), "rs.status()");
        }
    }

    void ExplorerReplicaSetFolderItem::handle(ReplicaSetFolderLoading *event)
    {
        setText(0, "Replica Set ...");
    }

    void ExplorerReplicaSetFolderItem::on_refresh()
    {
        _server->tryRefreshReplicaSetFolder(true);
    }
}

