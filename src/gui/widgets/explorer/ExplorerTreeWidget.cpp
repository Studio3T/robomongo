#include "ExplorerTreeWidget.h"
#include <QContextMenuEvent>
#include "AppRegistry.h"
//#include "ExplorerServerTreeItem.h"
#include <QMenu>
#include <QtGui>
#include "GuiRegistry.h"

using namespace Robomongo;

ExplorerTreeWidget::ExplorerTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::DefaultContextMenu);

    // Connect action
    QAction *disconnectAction = new QAction("Disconnect", this);
    disconnectAction->setIcon(GuiRegistry::instance().serverIcon());
    disconnectAction->setIconText("Disconnect");
    connect(disconnectAction, SIGNAL(triggered()), SLOT(ui_disconnectServer()));

    // Refresh action
    QAction *refreshAction = new QAction("Refresh", this);
    refreshAction->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    connect(refreshAction, SIGNAL(triggered()), SLOT(ui_refreshServer()));

    // File menu
    _serverMenu = new QMenu(this);
    _serverMenu->addAction(disconnectAction);
    _serverMenu->addSeparator();
    _serverMenu->addAction(refreshAction);
}

ExplorerTreeWidget::~ExplorerTreeWidget()
{
    int a = 67;
}

void ExplorerTreeWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->pos());
    if (!item)
        return;

//    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
//    if (!serverItem)
//        return;

//    _serverMenu->exec(mapToGlobal(event->pos()));
}

void ExplorerTreeWidget::ui_disconnectServer()
{
    emit disconnectActionTriggered();
}

void ExplorerTreeWidget::ui_refreshServer()
{
    emit refreshActionTriggered();
}
