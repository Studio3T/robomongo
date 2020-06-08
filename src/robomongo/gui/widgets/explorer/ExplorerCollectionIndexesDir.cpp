#include "ExplorerCollectionIndexesDir.h"

#include <QAction>
#include <QMenu>

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/widgets/explorer/EditIndexDialog.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"

namespace Robomongo
{
    ExplorerCollectionIndexesDir::ExplorerCollectionIndexesDir(QTreeWidgetItem *parent)
        :BaseClass(parent)
    {
        QAction *addIndex = new QAction("Add Index...", this);
        VERIFY(connect(addIndex, SIGNAL(triggered()), SLOT(ui_addIndex())));

        QAction *reIndex = new QAction("Rebuild Indexes...", this);
        VERIFY(connect(reIndex, SIGNAL(triggered()), SLOT(ui_reIndex())));

        QAction *viewIndex = new QAction("View Indexes", this);
        VERIFY(connect(viewIndex, SIGNAL(triggered()), SLOT(ui_viewIndex())));

        QAction *refreshIndex = new QAction("Refresh", this);
        VERIFY(connect(refreshIndex, SIGNAL(triggered()), SLOT(ui_refreshIndex())));

        BaseClass::_contextMenu->addAction(viewIndex);
        BaseClass::_contextMenu->addAction(addIndex);
        BaseClass::_contextMenu->addAction(reIndex);
        BaseClass::_contextMenu->addSeparator();
        BaseClass::_contextMenu->addAction(refreshIndex);

        setText(0, "Indexes");
        setIcon(0, Robomongo::GuiRegistry::instance().folderIcon());

        setExpanded(false);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void ExplorerCollectionIndexesDir::expand()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (!par)
            return;

        par->expand();
    }

    void ExplorerCollectionIndexesDir::ui_viewIndex()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (par) {
            par->openCurrentCollectionShell("getIndexes()");
        }
    }

    void ExplorerCollectionIndexesDir::ui_refreshIndex()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (par) {
            par->expand();
        }
    }

    void ExplorerCollectionIndexesDir::ui_addIndex()
    {
        auto par = dynamic_cast<ExplorerCollectionTreeItem *const>(parent());
        if (!par)
            return;

        IndexInfo fakeInfo(par->collection()->info(), "");
        EditIndexDialog dlg(fakeInfo, QtUtils::toQString(par->databaseItem()->database()->name()),
            QtUtils::toQString(par->databaseItem()->database()->server()->connectionRecord()->getFullAddress()),
            treeWidget());
        int result = dlg.exec();
        if (result != QDialog::Accepted)
            return;

        auto databaseTreeItem = static_cast<ExplorerDatabaseTreeItem *>(par->databaseItem());
        if (!databaseTreeItem)
            return;

        databaseTreeItem->addEditIndex(par, fakeInfo, dlg.info());
    }

    void ExplorerCollectionIndexesDir::ui_reIndex()
    {
        ExplorerCollectionTreeItem *par = dynamic_cast<ExplorerCollectionTreeItem *>(parent());
        if (par) {
            par->openCurrentCollectionShell("reIndex()", false);
        }
    }
}