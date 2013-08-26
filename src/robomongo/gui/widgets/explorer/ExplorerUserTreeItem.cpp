#include "robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/gui/dialogs/CreateUserDialog.h"
#include "robomongo/gui/utils/DialogUtils.h"
#include "robomongo/gui/GuiRegistry.h"

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    const QString tooltipTemplate = QString(
        "%0 "
        "<table>"
        "<tr><td>ID:</td> <td><b>&nbsp;&nbsp;%1</b></td></tr>"
        "<tr><td>Readonly:</td><td><b>&nbsp;&nbsp;%2</b></td></tr>"
        "</table>");

    QString buildToolTip(const Robomongo::MongoUser &user)
    {
        return tooltipTemplate.arg(user.name()).arg(QString::fromStdString(user.id().toString())).arg(user.readOnly() ? "Yes" : "No");
    }
}
namespace Robomongo
{
    ExplorerUserTreeItem::ExplorerUserTreeItem(QTreeWidgetItem *parent,MongoDatabase *const database, const MongoUser &user) :
        BaseClass(parent),_user(user),_database(database)
    {
        QAction *dropUser = new QAction("Drop User", this);
        connect(dropUser, SIGNAL(triggered()), SLOT(ui_dropUser()));

        QAction *editUser = new QAction("Edit User", this);
        connect(editUser, SIGNAL(triggered()), SLOT(ui_editUser()));

        BaseClass::_contextMenu->addAction(editUser);
        BaseClass::_contextMenu->addAction(dropUser);

        setText(0, _user.name());
        setIcon(0, GuiRegistry::instance().userIcon());
        setExpanded(false);
        //setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setToolTip(0, buildToolTip(user));
    }

    void ExplorerUserTreeItem::ui_dropUser()
    {
        // Ask user
        int answer = utils::questionDialog(treeWidget(),"Drop","User",_user.name());

        if (answer == QMessageBox::Yes){
            _database->dropUser(_user.id());
            _database->loadUsers(); // refresh list of users
        }
    }

    void ExplorerUserTreeItem::ui_editUser()
    {
        CreateUserDialog dlg(QtUtils::toQString(_database->server()->connectionRecord()->getFullAddress()),QtUtils::toQString(_database->name()),_user);
        dlg.setWindowTitle("Edit User");
        dlg.setUserPasswordLabelText("New Password:");
        int result = dlg.exec();

        if (result == QDialog::Accepted) {

            MongoUser user = dlg.user();
            _database->createUser(user, true);

            // refresh list of users
            _database->loadUsers();
        }
    }
}
