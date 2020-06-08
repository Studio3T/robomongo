#include "ExplorerCollectionIndexItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/DialogUtils.h"
#include "robomongo/gui/widgets/explorer/EditIndexDialog.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionIndexesDir.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"

namespace Robomongo
{
    ExplorerCollectionIndexItem::ExplorerCollectionIndexItem(
        ExplorerCollectionIndexesDir *parent, const IndexInfo &info)
        : BaseClass(parent), _info(info)
    {
        auto dropIndex = new QAction("Drop Index...", this);
        connect(dropIndex, SIGNAL(triggered()), SLOT(ui_dropIndex()));
        auto editIndex = new QAction("Edit Index...", this);
        connect(editIndex, SIGNAL(triggered()), SLOT(ui_edit()));

        BaseClass::_contextMenu->addAction(editIndex);
        BaseClass::_contextMenu->addAction(dropIndex);

        setText(0, QtUtils::toQString(_info._name));
        setIcon(0, Robomongo::GuiRegistry::instance().indexIcon());
    }

    void ExplorerCollectionIndexItem::ui_dropIndex()
    {
        // Ask user
        int answer = utils::questionDialog(treeWidget(), "Drop", "Index", text(0));

        if (answer != QMessageBox::Yes)
            return;

        ExplorerCollectionIndexesDir *par = dynamic_cast<ExplorerCollectionIndexesDir *>(parent());
        if (!par)
            return;
        
        ExplorerCollectionTreeItem *grandParent = dynamic_cast<ExplorerCollectionTreeItem *>(par->parent());
        if (!grandParent)
            return;

        grandParent->dropIndex(this);
    }

    void ExplorerCollectionIndexItem::ui_edit()
    {
        ExplorerCollectionIndexesDir *par = dynamic_cast<ExplorerCollectionIndexesDir *>(parent());           
        if (par) {
            ExplorerCollectionTreeItem *grPar = dynamic_cast<ExplorerCollectionTreeItem *const>(par->parent());
            if (!par) // todo: !grPar ?
                return;

            EditIndexDialog dlg(
                _info, QtUtils::toQString(grPar->databaseItem()->database()->name()), 
                QtUtils::toQString(grPar->databaseItem()->database()->server()->connectionRecord()->getFullAddress()), 
                treeWidget()
            );

            int result = dlg.exec();
            if (result != QDialog::Accepted)
                return;

            ExplorerDatabaseTreeItem *databaseTreeItem = static_cast<ExplorerDatabaseTreeItem *>(grPar->databaseItem());
            if (!databaseTreeItem)
                return;

            databaseTreeItem->addEditIndex(grPar, _info, dlg.info());
        }
    }
}