#include "robomongo/gui/widgets/explorer/ExplorerFunctionTreeItem.h"

#include <QAction>
#include <QMessageBox>

#include "robomongo/gui/dialogs/FunctionTextEditor.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"

namespace Robomongo
{

    ExplorerFunctionTreeItem::ExplorerFunctionTreeItem(QTreeWidgetItem *parent,MongoDatabase *database, const MongoFunction &function) :
        QObject(),
        BaseClass(parent),
        _function(function),
        _database(database)
    {

        QAction *dropFunction = new QAction("Remove Function", this);
        connect(dropFunction, SIGNAL(triggered()), SLOT(ui_dropFunction()));

        QAction *editFunction = new QAction("Edit Function", this);
        connect(editFunction, SIGNAL(triggered()), SLOT(ui_editFunction()));

        BaseClass::_contextMenu.addAction(editFunction);
        BaseClass::_contextMenu.addAction(dropFunction);

        setText(0, _function.name());
        setIcon(0, GuiRegistry::instance().functionIcon());
        setToolTip(0, buildToolTip(_function));
        setExpanded(false);
    }

    QString ExplorerFunctionTreeItem::buildToolTip(const MongoFunction &function)
    {
        QString tooltip = QString("%0")
            .arg(function.name());

        return tooltip;
    }

    void ExplorerFunctionTreeItem::ui_editFunction()
    {
        MongoFunction function = this->function();
        MongoDatabase *database = this->database();
        MongoServer *server = database->server();
        QString name = function.name();

        FunctionTextEditor dlg(server->connectionRecord()->getFullAddress(),
            database->name(),
            function);
        dlg.setWindowTitle("Edit Function");
        int result = dlg.exec();

        if (result == QDialog::Accepted) {

            MongoFunction editedFunction = dlg.function();
            database->updateFunction(name, editedFunction);

            // refresh list of functions
            database->loadFunctions();
        }
    }

    void ExplorerFunctionTreeItem::ui_dropFunction()
    {
        MongoFunction function = this->function();
        MongoDatabase *database = this->database();
    
    #pragma message ("TODO: Add parent to modal message")
        // Ask user
        int answer = QMessageBox::question(NULL,
            "Remove Function",
            QString("Remove <b>%1</b> function?").arg(function.name()),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        if (answer != QMessageBox::Yes)
            return;

        database->dropFunction(function.name());
        database->loadFunctions(); // refresh list of functions
    }
}
