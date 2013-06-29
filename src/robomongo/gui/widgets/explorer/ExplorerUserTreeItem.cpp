#include "robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h"

#include "robomongo/gui/GuiRegistry.h"

namespace Robomongo
{
    ExplorerUserTreeItem::ExplorerUserTreeItem(MongoDatabase *database, const MongoUser &user) :
        _user(user),
        _database(database)
    {
        setText(0, _user.name());
        setIcon(0, GuiRegistry::instance().userIcon());
        setExpanded(false);
        //setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setToolTip(0, buildToolTip(user));
    }

    QString ExplorerUserTreeItem::buildToolTip(const MongoUser &user)
    {
        QString tooltip = QString(
            "%0 "
            "<table>"
            "<tr><td>ID:</td> <td><b>&nbsp;&nbsp;%1</b></td></tr>"
            "<tr><td>Readonly:</td><td><b>&nbsp;&nbsp;%2</b></td></tr>"
            "</table>"
            )
            .arg(user.name())
            .arg(QString::fromStdString(user.id().toString()))
            .arg(user.readOnly() ? "Yes" : "No");

        return tooltip;
    }
}
