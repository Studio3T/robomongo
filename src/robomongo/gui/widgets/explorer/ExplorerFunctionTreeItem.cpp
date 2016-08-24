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
        std::string name = _function.name();

        EditWindowMode editWindowMode = AppRegistry::instance().settingsManager()->editWindowMode();
        if(editWindowMode != Tabbed) {
            FunctionTextEditor *dlg = new FunctionTextEditor(QtUtils::toQString(_database->server()->connectionRecord()->getFullAddress()),
                QtUtils::toQString(_database->name()),
                _function);
            dlg->setWindowTitle("Edit Function");
            switch(editWindowMode)
            {
                case Modal:
                    {
                        int result = dlg->exec();

                        if (result == QDialog::Accepted) {

                            MongoFunction editedFunction = dlg->function();
                            _database->updateFunction(name, editedFunction);

                            // refresh list of functions
                            _database->loadFunctions();
                        }
                    }
                    break;
                case Modeless:
                    VERIFY(connect(dlg, SIGNAL(accepted()), SLOT(functionTextEditorEditAccepted())));
                    dlg->show();
                    break;
            }
        } else {
            openCurrentCollectionShell(QtUtils::toQString("{\n"
                "    _id: \""+name+"\",\n"
                "    value : " + _function.code() + ""
                "}"), false);
        }
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

    void ExplorerFunctionTreeItem::functionTextEditorEditAccepted()
    {
        FunctionTextEditor *dlg = (FunctionTextEditor*)QObject::sender();
        MongoFunction editedFunction = dlg->function();
        _database->updateFunction(_function.name(), editedFunction);

        // refresh list of functions
        _database->loadFunctions();
    }

    void ExplorerFunctionTreeItem::openCurrentCollectionShell(const QString &script, bool execute, const CursorPosition &cursor)
    {
        QString query = "db.system.js.save(" + script + ")";
        AppRegistry::instance().app()->openShell(_database, query, execute, "Edit Function", cursor);
    }
}
