#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/gui/dialogs/FunctionTextEditor.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/DialogUtils.h"

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"

namespace Robomongo
{

    ExplorerFunctionTreeItem::ExplorerFunctionTreeItem(QTreeWidgetItem *parent, MongoDatabase *database, const MongoFunction &function) :
        BaseClass(parent),
        _function(function),
        _database(database)
    {

        QAction *dropFunction = new QAction("Remove Function", this);
        VERIFY(connect(dropFunction, SIGNAL(triggered()), SLOT(ui_dropFunction())));

        QAction *editFunction = new QAction("Edit Function", this);
        VERIFY(connect(editFunction, SIGNAL(triggered()), SLOT(ui_editFunction())));

        BaseClass::_contextMenu->addAction(editFunction);
        BaseClass::_contextMenu->addAction(dropFunction);

        setText(0, QtUtils::toQString(_function.name()));
        setIcon(0, GuiRegistry::instance().functionIcon());
        setToolTip(0, buildToolTip(_function));
        setExpanded(false);
    }

    QString ExplorerFunctionTreeItem::buildToolTip(const MongoFunction &function)
    {
        return QString("%0").arg(QtUtils::toQString(function.name()));
    }

    void ExplorerFunctionTreeItem::ui_editFunction()
    {
        ExplorerDatabaseCategoryTreeItem *dcti = (ExplorerDatabaseCategoryTreeItem*)parent();
        dcti->ui_editFunction(_function);
    }

    void ExplorerFunctionTreeItem::ui_dropFunction()
    {
        // Ask user
        int answer = utils::questionDialog(treeWidget(), "Drop", "Function", QtUtils::toQString(_function.name()));

        if (answer != QMessageBox::Yes)
            return;

        _database->dropFunction(_function.name());
        _database->loadFunctions(); // refresh list of functions
    }
}
