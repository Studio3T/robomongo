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
#include "robomongo/gui/GuiRegistry.h"

namespace
{
    void openCurrentServerShell(Robomongo::MongoServer *const server, const QString &script, bool execute = true, 
                                const Robomongo::CursorPosition &cursor = Robomongo::CursorPosition())
    {
        Robomongo::AppRegistry::instance().app()->openShell(server, script, std::string(), execute, 
            Robomongo::QtUtils::toQString(server->connectionRecord()->getReadableName()), cursor);
    }

    // todo : modify or move to a common header
    void openCurrentServerShell(Robomongo::ConnectionSettings* connSettings, const QString &script)
    {
        auto const scriptStr = Robomongo::ScriptInfo(script, true);
        Robomongo::AppRegistry::instance().app()->openShell(connSettings, scriptStr);
    }
}

namespace Robomongo
{

    ExplorerReplicaSetFolderItem::ExplorerReplicaSetFolderItem(ExplorerTreeItem *parent, MongoServer *const server) :
        BaseClass(parent), _server(server)
    {
        //VERIFY(connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(on_expanded())));

        auto repSetStatus = new QAction("Status of Replica Set", this);
        VERIFY(connect(repSetStatus, SIGNAL(triggered()), SLOT(on_repSetStatus())));

        auto refresh = new QAction("Refresh", this);
        VERIFY(connect(refresh, SIGNAL(triggered()), SLOT(on_refresh())));

        BaseClass::_contextMenu->addAction(repSetStatus);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(refresh);

        AppRegistry::instance().bus()->subscribe(this, ReplicaSetFolderLoading::Type, _server);

        setIcon(0, GuiRegistry::instance().folderIcon());
        // todo: use repSize()
        setText(0, "Replica Set (" + QString::number(_server->replicaSetInfo()->membersAndHealths.size()) + " nodes)");

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerReplicaSetFolderItem::updateText()
    {
        setText(0, "Replica Set (" + QString::number(_server->replicaSetInfo()->membersAndHealths.size()) + " nodes)");
    }

    void ExplorerReplicaSetFolderItem::disableSomeContextMenuActions(/*bool disable*/)
    {
        // todo: refactor
        if (BaseClass::_contextMenu->actions().size() < 1)
            return;

        // Break if there is at least one online set member  
        mongo::HostAndPort onlineMember;
        for (auto const& member : _server->replicaSetInfo()->membersAndHealths) {
            if (member.second) {
                onlineMember = mongo::HostAndPort(member.first);
                break;
            }
        }

        // Show menu item "Status of Replica Set" only if there is at least one online member
        if (onlineMember.empty()) 
            BaseClass::_contextMenu->actions().at(0)->setDisabled(true);
        else
            BaseClass::_contextMenu->actions().at(0)->setDisabled(false);
    }

    void ExplorerReplicaSetFolderItem::expand()
    {
        if (!_refreshFlag) {
            _refreshFlag = true;
            return;
        }
        _server->tryRefreshReplicaSetFolder();
    }

    void ExplorerReplicaSetFolderItem::on_repSetStatus()
    {
        if (!_server->replicaSetInfo()->primary.empty()) {
            openCurrentServerShell(_server, "rs.status()");
        }
        else // primary is unreachable
        {
            // Todo: do this before 
            // Run rs.status only if there is a reachable secondary
            mongo::HostAndPort onlineMember;
            for (auto const& member : _server->replicaSetInfo()->membersAndHealths) {
                if (member.second) {
                    onlineMember = mongo::HostAndPort(member.first);
                    break;
                }
            }
            if (onlineMember.empty())   // todo: throw error
                return;

            auto connSetting = _server->connectionRecord()->clone();    // todo: unique_ptr
            // Set connection settings of this replica member
            connSetting->setConnectionName(onlineMember.toString() + 
                                           " [member of " + connSetting->connectionName() + "]");
            connSetting->setServerHost(onlineMember.host());
            connSetting->setServerPort(onlineMember.port());
            connSetting->setReplicaSet(false);
            connSetting->replicaSetSettings()->setMembers(std::vector<std::string>()); 

            openCurrentServerShell(connSetting, "rs.status()");
        }
    }

    void ExplorerReplicaSetFolderItem::handle(ReplicaSetFolderLoading *event)
    {
        setText(0, "Replica Set ...");
    }

    void ExplorerReplicaSetFolderItem::on_refresh()
    {
        _server->tryRefreshReplicaSetFolder();
    }
}

