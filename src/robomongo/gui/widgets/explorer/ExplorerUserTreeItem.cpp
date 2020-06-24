#include "robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/gui/dialogs/CreateUserDialog.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/DialogUtils.h"

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    ExplorerUserTreeItem::ExplorerUserTreeItem(
        QTreeWidgetItem *parent, MongoDatabase *const database, const MongoUser &user) 
        : BaseClass(parent), _user(user), _database(database)
    {
        auto const dropUser { new QAction("Drop User", this) };
        VERIFY(connect(dropUser, SIGNAL(triggered()), SLOT(ui_dropUser())));

        auto const viewUser { new QAction("View User", this) };
        VERIFY(connect(viewUser, SIGNAL(triggered()), SLOT(ui_viewUser())));

        BaseClass::_contextMenu->addAction(viewUser);
        BaseClass::_contextMenu->addAction(dropUser);

        setText(0, QtUtils::toQString(_user.name()));
        setIcon(0, GuiRegistry::instance().userIcon());
        setExpanded(false);
    }

    void ExplorerUserTreeItem::ui_dropUser()
    {
        // Ask user
        int const answer {
            utils::questionDialog(
                treeWidget(), "Drop", "User", QtUtils::toQString(_user.name())
            )
        };

        if (answer == QMessageBox::Yes)
            _database->dropUser(_user.name());
    }

    void ExplorerUserTreeItem::ui_viewUser()
    {
        auto const& app { Robomongo::AppRegistry::instance().app() };
        app->openShell(
            _database, QString::fromStdString("db.getUser(\"" + _user.name() + "\")"), true,
            QtUtils::toQString(_database->name()), CursorPosition()
        );
    }
}
