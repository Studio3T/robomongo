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
    const char* tooltipTemplate = 
        "%s "
        "<table>"
        "<tr><td>ID:</td><td width=\"180\"><b>&nbsp;&nbsp;%s</b></td></tr>"
        "</table>";

    std::string buildToolTip(const Robomongo::MongoUser &user)
    {
        char buff[2048] = {0};
        sprintf(buff, tooltipTemplate, user.name().c_str(), user.id().toString().c_str());
        return buff;
    }
}

namespace Robomongo
{
    ExplorerUserTreeItem::ExplorerUserTreeItem(QTreeWidgetItem *parent, MongoDatabase *const database, const MongoUser &user) :
        BaseClass(parent), _user(user), _database(database)
    {
        QAction *dropUser = new QAction("Drop User", this);
        VERIFY(connect(dropUser, SIGNAL(triggered()), SLOT(ui_dropUser())));

        QAction *editUser = new QAction("Edit User", this);
        VERIFY(connect(editUser, SIGNAL(triggered()), SLOT(ui_editUser())));

        BaseClass::_contextMenu->addAction(editUser);
        BaseClass::_contextMenu->addAction(dropUser);

        setText(0, QtUtils::toQString(_user.name()));
        setIcon(0, GuiRegistry::instance().userIcon());
        setExpanded(false);
        //setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setToolTip(0, QtUtils::toQString(buildToolTip(user)));
    }

    void ExplorerUserTreeItem::ui_dropUser()
    {
        // Ask user
        int const answer = utils::questionDialog(treeWidget(), "Drop", "User", 
                                                 QtUtils::toQString(_user.name()));

        if (answer == QMessageBox::Yes)
            _database->dropUser(_user.id(), _user.name());
    }

    void ExplorerUserTreeItem::ui_editUser()
    {
        std::unique_ptr<CreateUserDialog> dlg = nullptr;

        float const version = _user.version();
        if (version < MongoUser::minimumSupportedVersion) {
            dlg.reset(new CreateUserDialog(
                QtUtils::toQString(_database->server()->connectionRecord()->getFullAddress()), 
                QtUtils::toQString(_database->name()), _user, treeWidget()));
        }
        else {
           dlg.reset(new CreateUserDialog(_database->server()->getDatabasesNames(), 
               QtUtils::toQString(_database->server()->connectionRecord()->getFullAddress()), 
               QtUtils::toQString(_database->name()), _user, treeWidget()));
        }
        
        dlg->setWindowTitle("Edit User");
        dlg->setUserPasswordLabelText("New Password:");

        if (dlg->exec() == QDialog::Accepted) {
            MongoUser user = dlg->user();
            _database->createUser(user, true);
        }
    }
}
