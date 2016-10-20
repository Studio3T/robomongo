#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetFolderItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    void openCurrentServerShell(Robomongo::MongoServer *const server, const QString &script, bool execute = true, 
                                const Robomongo::CursorPosition &cursor = Robomongo::CursorPosition())
    {
        Robomongo::AppRegistry::instance().app()->openShell(server, script, std::string(), execute, 
            Robomongo::QtUtils::toQString(server->connectionRecord()->getReadableName()), cursor);
    }
}

namespace Robomongo
{

    ExplorerReplicaSetFolderItem::ExplorerReplicaSetFolderItem(ExplorerTreeItem *parent, MongoServer *const server) :
        BaseClass(parent), _server(server)
    {
        //VERIFY(connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(on_expanded())));

        auto repSetStatus = new QAction("Status", this);
        VERIFY(connect(repSetStatus, SIGNAL(triggered()), SLOT(on_repSetStatus())));

        auto refresh = new QAction("Refresh", this);
        VERIFY(connect(refresh, SIGNAL(triggered()), SLOT(on_refresh())));

        BaseClass::_contextMenu->addAction(repSetStatus);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(refresh);

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerReplicaSetFolderItem::on_repSetStatus()
    {
        openCurrentServerShell(_server, "rs.status()");
    }

    void ExplorerReplicaSetFolderItem::on_refresh()
    {
        _server->tryRefresh();
        _server->loadDatabases();
    }

}

