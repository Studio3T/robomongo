#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetFolderItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/AppRegistry.h"
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

    void ExplorerReplicaSetFolderItem::on_repSetStatus()
    {
        openCurrentServerShell(_server, "rs.status()");
    }

    void ExplorerReplicaSetFolderItem::on_refresh()
    {
        setText(0, "Replica Set ...");
        
        //_server->tryRefresh();      // todo: is it needed?
        _server->tryRefreshReplicaSet();
        //_server->loadDatabases();   // todo: refactor
    }
}

