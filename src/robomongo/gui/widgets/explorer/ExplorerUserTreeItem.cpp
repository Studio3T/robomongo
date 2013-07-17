#include "robomongo/gui/widgets/explorer/ExplorerUserTreeItem.h"

#include <QAction>
#include <QMessageBox>

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/CreateUserDialog.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"

namespace
{
	const QString tooltipTemplate = QString(
		"%0 "
		"<table>"
		"<tr><td>ID:</td> <td><b>&nbsp;&nbsp;%1</b></td></tr>"
		"<tr><td>Readonly:</td><td><b>&nbsp;&nbsp;%2</b></td></tr>"
		"</table>");
}
namespace Robomongo
{
    ExplorerUserTreeItem::ExplorerUserTreeItem(QTreeWidgetItem *parent,MongoDatabase *database, const MongoUser &user) :
        QObject(),
        BaseClass(parent),
        _user(user),
        _database(database)
    {
        QAction *dropUser = new QAction("Remove User", this);
        connect(dropUser, SIGNAL(triggered()), SLOT(ui_dropUser()));

        QAction *editUser = new QAction("Edit User", this);
        connect(editUser, SIGNAL(triggered()), SLOT(ui_editUser()));

        BaseClass::_contextMenu.addAction(editUser);
        BaseClass::_contextMenu.addAction(dropUser);

        setText(0, _user.name());
        setIcon(0, GuiRegistry::instance().userIcon());
        setExpanded(false);
        //setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setToolTip(0, buildToolTip(user));
    }

    QString ExplorerUserTreeItem::buildToolTip(const MongoUser &user)
    {       
        return tooltipTemplate.arg(user.name()).arg(QString::fromStdString(user.id().toString())).arg(user.readOnly() ? "Yes" : "No");
    }

    void ExplorerUserTreeItem::ui_dropUser()
    {
            // Ask user
    #pragma message ("TODO: Add parent to modal message")
            int answer = QMessageBox::question(NULL,
                "Remove User",
                QString("Remove <b>%1</b> user?").arg(user().name()),
                QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

            if (answer == QMessageBox::Yes){
                MongoDatabase *database = this->database();
                database->dropUser(user().id());
                database->loadUsers(); // refresh list of users
            }
    }

    void ExplorerUserTreeItem::ui_editUser()
    {
        MongoDatabase *database = ExplorerUserTreeItem::database();

        CreateUserDialog dlg(database->server()->connectionRecord()->getFullAddress(),
            database->name(),
            user());
        dlg.setWindowTitle("Edit User");
        dlg.setUserPasswordLabelText("New Password:");
        int result = dlg.exec();

        if (result == QDialog::Accepted) {

            MongoUser user = dlg.user();
            database->createUser(user, true);

            // refresh list of users
            database->loadUsers();
        }
    }
}
