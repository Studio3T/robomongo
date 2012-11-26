#include <QtGui>
#include "ExplorerWidget.h"
#include "ExplorerDatabaseTreeItem.h"
#include "ExplorerDatabaseCategoryTreeItem.h"
#include "AppRegistry.h"
#include "ExplorerTreeWidget.h"
#include "ExplorerServerTreeItem.h"
#include "Dispatcher.h"
#include "ExplorerCollectionTreeItem.h"

using namespace Robomongo;

/**
 * Constructs ExplorerWidget
 */
ExplorerWidget::ExplorerWidget(QWidget *parent) : QWidget(parent),
    _progress(0),
    _dispatcher(AppRegistry::instance().dispatcher())
{
    _treeWidget = new ExplorerTreeWidget;
    _treeWidget->setIndentation(15);    
    _treeWidget->setHeaderHidden(true);

    QHBoxLayout *vlaout = new QHBoxLayout();
    vlaout->setMargin(0);
    vlaout->addWidget(_treeWidget, Qt::AlignJustify);

    connect(_treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem *)), SLOT(ui_itemExpanded(QTreeWidgetItem *)));
    connect(_treeWidget, SIGNAL(customContextMenuRequested(QPoint)), SLOT(ui_customContextMenuRequested(QPoint)));
    connect(_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(ui_itemClicked(QTreeWidgetItem*,int)));
    connect(_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT(ui_itemDoubleClicked(QTreeWidgetItem *, int)));
    connect(_treeWidget, SIGNAL(disconnectActionTriggered()), SLOT(ui_disonnectActionTriggered()));

    setLayout(vlaout);

    QMovie *movie = new QMovie(":robomongo/icons/loading.gif", QByteArray(), this);
    _progressLabel = new QLabel(this);
    _progressLabel->setMovie(movie);
    _progressLabel->hide();
    movie->start();

    _dispatcher.subscribe(this, ConnectingEvent::EventType);
    _dispatcher.subscribe(this, ConnectionFailedEvent::EventType);
    _dispatcher.subscribe(this, ConnectionEstablishedEvent::EventType);
}

bool ExplorerWidget::event(QEvent *event)
{
    R_HANDLE(event) {
        R_EVENT(ConnectingEvent);
        R_EVENT(ConnectionFailedEvent);
        R_EVENT(ConnectionEstablishedEvent);
    }

    return QWidget::event(event);
}

void ExplorerWidget::ui_disonnectActionTriggered()
{
    QList<QTreeWidgetItem*> items = _treeWidget->selectedItems();

    if (items.count() != 1)
        return;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return;

//    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
//    if (!serverItem)
//        return;

//    _viewModel->removeServer(serverItem->viewModel());

//    int index = _treeWidget->indexOfTopLevelItem(serverItem);

    //    _treeWidget->takeTopLevelItem(index);
}

void ExplorerWidget::increaseProgress()
{
    ++_progress;
    _progressLabel->move(width() / 2 - 8, height() / 2 - 8);
    _progressLabel->show();
}

void ExplorerWidget::decreaseProgress()
{
    --_progress;

    if (!_progress)
        _progressLabel->hide();
}

void ExplorerWidget::ui_itemClicked(QTreeWidgetItem* a1 ,int b)
{
    int a = b;
}

void ExplorerWidget::ui_customContextMenuRequested(QPoint p)
{
    int a = p.rx();
}

void ExplorerWidget::handle(ConnectingEvent *event)
{
    increaseProgress();
}

void ExplorerWidget::handle(ConnectionEstablishedEvent *event)
{
    decreaseProgress();

    QTreeWidgetItem *item = new ExplorerServerTreeItem(event->server);
    _treeWidget->addTopLevelItem(item);

//    _treeWidget->setItemWidget(item, 0, yourLabel);
}

void ExplorerWidget::handle(ConnectionFailedEvent *event)
{
    decreaseProgress();
}

void ExplorerWidget::removeServer()
{

}

/*
** Handle item expanding
*/
void ExplorerWidget::ui_itemExpanded(QTreeWidgetItem *item)
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = dynamic_cast<ExplorerDatabaseCategoryTreeItem *>(item);
    if (categoryItem)
    {
        switch(categoryItem->category())
        {
            case Collections:
                categoryItem->databaseItem()->expandCollections();
                break;
            case Files:
                break;
            case Functions:
                break;
            case Users:
                break;
        }

        return;
    }


    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
    if (serverItem)
    {
        serverItem->expand();
        return;
    }
}

/*
** Handle item double clicking
*/
void ExplorerWidget::ui_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    ExplorerCollectionTreeItem *collectionItem = dynamic_cast<ExplorerCollectionTreeItem *>(item);
    if (collectionItem)
    {
//        QueryCreatedEvent *evnt = new QueryCreatedEvent(collectionItem->viewModel()->collection());
//        AppRegistry::instance().mediator()->emitQueryCreated(evnt);
    }
}
