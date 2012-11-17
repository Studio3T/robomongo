#include "ExplorerWidget.h"
//#include "ExplorerServerTreeItem.h"
//#include "ExplorerDatabaseTreeItem.h"
//#include "ExplorerDatabaseCategoryTreeItem.h"
//#include "ExplorerCollectionTreeItem.h"

#include "AppRegistry.h"
#include <QtGui>
#include "ExplorerTreeWidget.h"

using namespace Robomongo;

/*
** Constructs ExplorerWidget
*/
ExplorerWidget::ExplorerWidget(QWidget *parent) : QWidget(parent)
{
    //_viewModel = viewModel;
    //connect(_viewModel, SIGNAL(serverAdded(ExplorerServerViewModel *)), this, SLOT(addServer(ExplorerServerViewModel *)));

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

void ExplorerWidget::ui_itemClicked(QTreeWidgetItem* a1 ,int b)
{
    int a = b;
}

void ExplorerWidget::ui_customContextMenuRequested(QPoint p)
{
    int a = p.rx();
}

/*
** Add server to tree view
*/
void ExplorerWidget::addServer()
{
//    QTreeWidgetItem *item = new ExplorerServerTreeItem(server);
//    _treeWidget->addTopLevelItem(item);
}

void ExplorerWidget::removeServer()
{

}

/*
** Handle item expanding
*/
void ExplorerWidget::ui_itemExpanded(QTreeWidgetItem *item)
{
    /*
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
    } */
}

/*
** Handle item double clicking
*/
void ExplorerWidget::ui_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    /*
    ExplorerCollectionTreeItem *collectionItem = dynamic_cast<ExplorerCollectionTreeItem *>(item);
    if (collectionItem)
    {
        QueryCreatedEvent *evnt = new QueryCreatedEvent(collectionItem->viewModel()->collection());
        AppRegistry::instance().mediator()->emitQueryCreated(evnt);
    }*/
}
