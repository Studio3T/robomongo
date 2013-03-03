#include "robomongo/gui/widgets/explorer/ExplorerWidget.h"

#include <QtGui>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeWidget.h"
#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"

using namespace Robomongo;

ExplorerWidget::ExplorerWidget(QWidget *parent) : QWidget(parent),
    _progress(0),
    _bus(AppRegistry::instance().bus()),
    _app(AppRegistry::instance().app())
{
    _treeWidget = new ExplorerTreeWidget;
    _treeWidget->setIndentation(15);    
    _treeWidget->setHeaderHidden(true);
    _treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    QHBoxLayout *vlaout = new QHBoxLayout();
    vlaout->setMargin(0);
    vlaout->addWidget(_treeWidget, Qt::AlignJustify);

    connect(_treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem *)), SLOT(ui_itemExpanded(QTreeWidgetItem *)));
    connect(_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT(ui_itemDoubleClicked(QTreeWidgetItem *, int)));
    connect(_treeWidget, SIGNAL(disconnectActionTriggered()), SLOT(ui_disonnectActionTriggered()));
    connect(_treeWidget, SIGNAL(openShellActionTriggered()), SLOT(ui_openShellActionTriggered()));

    setLayout(vlaout);

    QMovie *movie = new QMovie(":robomongo/icons/loading.gif", QByteArray(), this);
    _progressLabel = new QLabel(this);
    _progressLabel->setMovie(movie);
    _progressLabel->hide();
    movie->start();

    _bus->subscribe(this, ConnectingEvent::Type);
    _bus->subscribe(this, ConnectionFailedEvent::Type);
    _bus->subscribe(this, ConnectionEstablishedEvent::Type);
}

void ExplorerWidget::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter))
    {
        QList<QTreeWidgetItem*> items = _treeWidget->selectedItems();

        if (items.count() != 1) {
            QWidget::keyPressEvent(event);
            return;
        }

        QTreeWidgetItem *item = items[0];

        if (!item) {
            QWidget::keyPressEvent(event);
            return;
        }

        ui_itemDoubleClicked(item, 0);

        return;
    }

    QWidget::keyPressEvent(event);
}

void ExplorerWidget::ui_disonnectActionTriggered()
{
    QList<QTreeWidgetItem*> items = _treeWidget->selectedItems();

    if (items.count() != 1)
        return;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return;

    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
    if (!serverItem)
        return;

    int index = _treeWidget->indexOfTopLevelItem(serverItem);
    if (index != -1) {
        QTreeWidgetItem *removedItem = _treeWidget->takeTopLevelItem(index);
        if (removedItem) {
            MongoServer *server = serverItem->server();

            delete removedItem;
            _app->closeServer(server);
        }
    }
}

void ExplorerWidget::ui_openShellActionTriggered()
{
    QList<QTreeWidgetItem*> items = _treeWidget->selectedItems();

    if (items.count() != 1)
        return;

    QTreeWidgetItem *item = items[0];

    if (!item)
        return;

    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
    if (!serverItem)
        return;

    _app->openShell(serverItem->server(), "");
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

    if (_progress < 0)
        _progress = 0;

    if (!_progress)
        _progressLabel->hide();
}

void ExplorerWidget::handle(ConnectingEvent *event)
{
    increaseProgress();
}

void ExplorerWidget::handle(ConnectionEstablishedEvent *event)
{
    decreaseProgress();

    ExplorerServerTreeItem *item = new ExplorerServerTreeItem(event->server);
    item->setExpanded(true);
    _treeWidget->addTopLevelItem(item);
    _treeWidget->setCurrentItem(item);
    _treeWidget->setFocus();
}

void ExplorerWidget::handle(ConnectionFailedEvent *event)
{
    decreaseProgress();
}

void ExplorerWidget::ui_itemExpanded(QTreeWidgetItem *item)
{
    ExplorerDatabaseCategoryTreeItem *categoryItem = dynamic_cast<ExplorerDatabaseCategoryTreeItem *>(item);
    if (categoryItem) {
        switch(categoryItem->category()) {
            case Collections:
                categoryItem->databaseItem()->expandCollections();
                break;
            case Files:
                break;
            case Functions:
                break;
            case Users:
                categoryItem->databaseItem()->expandUsers();
                break;
        }
        return;
    }

    ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
    if (serverItem) {
        serverItem->expand();
        return;
    }
}

void ExplorerWidget::ui_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    ExplorerCollectionTreeItem *collectionItem = dynamic_cast<ExplorerCollectionTreeItem *>(item);
    if (collectionItem) {
        _app->openShell(collectionItem->collection());
    }
}
