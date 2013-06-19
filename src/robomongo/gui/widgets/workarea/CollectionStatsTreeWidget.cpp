#include "robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.h"

#include <QHeaderView>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/gui/widgets/workarea/CollectionStatsTreeItem.h"

using namespace Robomongo;

CollectionStatsTreeWidget::CollectionStatsTreeWidget(MongoShell *shell, QWidget *parent) : QTreeWidget(parent),
    _shell(shell),
    _bus(AppRegistry::instance().bus())
{
    QStringList colums;
    colums << "Name" << "Count" << "Size" << "Storage" << "Index" << "Average Object" << "Padding";
    setHeaderLabels(colums);

    setStyleSheet(
        "QTreeWidget { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }"
    );
}

void CollectionStatsTreeWidget::setDocuments(const QList<MongoDocumentPtr> &documents)
{
    _documents = documents;

    setUpdatesEnabled(false);
    clear();

    CollectionStatsTreeItem *firstItem = NULL;

    QList<QTreeWidgetItem *> items;
    for (int i = 0; i < documents.count(); i++)
    {
        MongoDocumentPtr document = documents.at(i);

        CollectionStatsTreeItem *item = new CollectionStatsTreeItem(document);
        items.append(item);

        if (i == 0)
            firstItem = item;
    }

    addTopLevelItems(items);

    header()->resizeSections(QHeaderView::ResizeToContents);
    setUpdatesEnabled(true);
}
