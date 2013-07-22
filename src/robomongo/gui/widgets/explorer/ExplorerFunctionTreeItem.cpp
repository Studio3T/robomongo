#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/gui/dialogs/FunctionTextEditor.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/DialogUtils.h"

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"

namespace Robomongo
{

    ExplorerFunctionTreeItem::ExplorerFunctionTreeItem(QTreeWidgetItem *parent,MongoDatabase *database, const MongoFunction &function) :
        BaseClass(parent),
        _function(function),
        _database(database)
    {

        QAction *dropFunction = new QAction("Remove Function", this);
        connect(dropFunction, SIGNAL(triggered()), SLOT(ui_dropFunction()));

        QAction *editFunction = new QAction("Edit Function", this);
        connect(editFunction, SIGNAL(triggered()), SLOT(ui_editFunction()));

        BaseClass::_contextMenu->addAction(editFunction);
        BaseClass::_contextMenu->addAction(dropFunction);

        setText(0, _function.name());
        setIcon(0, GuiRegistry::instance().functionIcon());
        setToolTip(0, buildToolTip(_function));
        setExpanded(false);
    }

    QString ExplorerFunctionTreeItem::buildToolTip(const MongoFunction &function)
    {
        return QString("%0").arg(function.name());
    }

    void ExplorerFunctionTreeItem::ui_editFunction()
    {
        QString name = _function.name();

        FunctionTextEditor dlg(_database->server()->connectionRecord()->getFullAddress(),
            _database->name(),
            _function);
        dlg.setWindowTitle("Edit Function");
        int result = dlg.exec();

        if (result == QDialog::Accepted) {

            MongoFunction editedFunction = dlg.function();
            _database->updateFunction(name, editedFunction);

            // refresh list of functions
            _database->loadFunctions();
        }
    }

    void ExplorerFunctionTreeItem::ui_dropFunction()
    {
        // Ask user
        int answer = utils::questionDialog(treeWidget(),"Drop","Function",_function.name());

        if (answer != QMessageBox::Yes)
            return;

        _database->dropFunction(_function.name());
        _database->loadFunctions(); // refresh list of functions
    }
}
