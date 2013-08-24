#include "robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.h"

#include <QHeaderView>

#include "robomongo/gui/widgets/workarea/CollectionStatsTreeItem.h"

namespace Robomongo
{

    CollectionStatsTreeWidget::CollectionStatsTreeWidget(MongoShell *shell, QWidget *parent) 
        : QTreeWidget(parent)
    {
        QStringList colums;
        colums << "Name" << "Count" << "Size" << "Storage" << "Index" << "Average Object" << "Padding";
        setHeaderLabels(colums);

        setStyleSheet(
            "QTreeWidget { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }"
        );
    }

    void CollectionStatsTreeWidget::setDocuments(const std::vector<MongoDocumentPtr> &documents)
    {
        setUpdatesEnabled(false);
        clear();

        QList<QTreeWidgetItem *> items;
        size_t documentsCount = documents.size();
        for (int i = 0; i < documentsCount; i++){
            MongoDocumentPtr document = documents[i];
            CollectionStatsTreeItem *item = new CollectionStatsTreeItem(document);
            items.append(item);
        }

        addTopLevelItems(items);

        header()->resizeSections(QHeaderView::ResizeToContents);
        setUpdatesEnabled(true);
    }
}
