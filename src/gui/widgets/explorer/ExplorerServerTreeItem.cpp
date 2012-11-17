#include "StdAfx.h"
#include "ExplorerServerTreeItem.h"
#include "ExplorerServerViewModel.h"
#include "ExplorerDatabaseViewModel.h"
#include "ExplorerDatabaseTreeItem.h"
#include "AppRegistry.h"

ExplorerServerTreeItem::ExplorerServerTreeItem(ExplorerServerViewModel * viewModel) : QObject()
{
	_viewModel = viewModel;

	setText(0, _viewModel->serverName());
	setIcon(0, AppRegistry::instance().serverIcon());
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

	connect(_viewModel, SIGNAL(databasesRefreshed(QList<ExplorerDatabaseViewModel *>)), SLOT(databaseRefreshed(QList<ExplorerDatabaseViewModel *>)));
}

void ExplorerServerTreeItem::expand()
{
	_viewModel->expand();
}

void ExplorerServerTreeItem::databaseRefreshed(QList<ExplorerDatabaseViewModel *> databases)
{
    // Remove child items
	int itemCount = childCount();
	for (int i = 0; i < itemCount; ++i)
	{
		QTreeWidgetItem * p = child(0);
		removeChild(p);
		delete p;
	}

    // Add system folder
    QIcon folderIcon = AppRegistry::instance().folderIcon();
    QTreeWidgetItem * systemFolder = new QTreeWidgetItem();
    systemFolder->setIcon(0, folderIcon);
    systemFolder->setText(0, "System");
    addChild(systemFolder);

	foreach(ExplorerDatabaseViewModel * database, databases)
	{
        if (database->system())
        {
            ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(database);
            systemFolder->addChild(dbItem);
            continue;
        }

		ExplorerDatabaseTreeItem * dbItem = new ExplorerDatabaseTreeItem(database);
		addChild(dbItem);
	}	
}
