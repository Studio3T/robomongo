#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"

#include <QAction>
#include <QMenu>

#include "robomongo/gui/dialogs/FunctionTextEditor.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/utils/DialogUtils.h"

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{

    ExplorerFunctionTreeItem::ExplorerFunctionTreeItem(QTreeWidgetItem *parent,MongoDatabase *database, const MongoFunction &function) :
        BaseClass(parent),
        _function(function),
        _database(database)
    {

        _dropFunctionAction = new QAction(this);
        VERIFY(connect(_dropFunctionAction, SIGNAL(triggered()), SLOT(ui_dropFunction())));

        _editFunctionAction = new QAction(this);
        VERIFY(connect(_editFunctionAction, SIGNAL(triggered()), SLOT(ui_editFunction())));

        BaseClass::_contextMenu->addAction(_editFunctionAction);
        BaseClass::_contextMenu->addAction(_dropFunctionAction);

        setText(0, QtUtils::toQString(_function.name()));
        setIcon(0, GuiRegistry::instance().functionIcon());
        setToolTip(0, buildToolTip(_function));
        setExpanded(false);
        
        retranslateUI();
    }
    
    void ExplorerFunctionTreeItem::retranslateUI()
    {
        _dropFunctionAction->setText(tr("Remove Function"));
        _editFunctionAction->setText(tr("Edit Function"));
    }

    QString ExplorerFunctionTreeItem::buildToolTip(const MongoFunction &function)
    {
        return QString("%0").arg(QtUtils::toQString(function.name()));
    }

    void ExplorerFunctionTreeItem::ui_editFunction()
    {
        std::string name = _function.name();

        FunctionTextEditor dlg(QtUtils::toQString(_database->server()->connectionRecord()->getFullAddress()),
            QtUtils::toQString(_database->name()),
            _function);
        dlg.setWindowTitle(tr("Edit Function"));
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
        int answer = utils::questionDialog(treeWidget(),tr("Drop"),tr("Function"),QtUtils::toQString(_function.name()));

        if (answer != QMessageBox::Yes)
            return;

        _database->dropFunction(_function.name());
        _database->loadFunctions(); // refresh list of functions
    }
}
