#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"

#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

ExplorerFunctionTreeItem::ExplorerFunctionTreeItem(MongoDatabase *database, const MongoFunction &function) :
    _function(function),
    _database(database)
{
    setText(0, _function.name());
    setIcon(0, GuiRegistry::instance().userIcon());
    setToolTip(0, buildToolTip(_function));
    setExpanded(false);
}

QString ExplorerFunctionTreeItem::buildToolTip(const MongoFunction &function)
{
    QString tooltip = QString("%0")
        .arg(function.name());

    return tooltip;
}
