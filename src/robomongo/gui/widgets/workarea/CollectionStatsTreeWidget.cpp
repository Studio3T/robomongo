#include "robomongo/gui/widgets/workarea/CollectionStatsTreeWidget.h"

#include <QHeaderView>

#include "robomongo/gui/widgets/workarea/CollectionStatsTreeItem.h"

namespace Robomongo
{

    CollectionStatsTreeWidget::CollectionStatsTreeWidget(const std::vector<MongoDocumentPtr> &documents, QWidget *parent) 
        : QTreeWidget(parent)
    {
        setStyleSheet(
            "QTreeWidget { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }"
        );

        QList<QTreeWidgetItem *> items;
        size_t documentsCount = documents.size();
        for (int i = 0; i < documentsCount; i++) {
            MongoDocumentPtr document = documents[i];
            CollectionStatsTreeItem *item = new CollectionStatsTreeItem(document);
            items.append(item);
        }

        addTopLevelItems(items);
        
        retranslateUI();

        header()->resizeSections(QHeaderView::ResizeToContents);
    }

    void CollectionStatsTreeWidget::retranslateUI()
    {
        QStringList colums;
        colums << tr("Name") << tr("Count") << tr("Size") << tr("Storage") << tr("Index") << tr("Average Object") << tr("Padding");
        setHeaderLabels(colums);
    }
    
    void CollectionStatsTreeWidget::changeEvent(QEvent* event)
    {
        if (0 != event) {
            switch (event->type()) {
                // this event is send if a translator is loaded
            case QEvent::LanguageChange:
                retranslateUI();
                break;
                // this event is send, if the system, language changes
            case QEvent::LocaleChange:
                break;
            }
        }
        BaseClass::changeEvent(event);
    }
}
