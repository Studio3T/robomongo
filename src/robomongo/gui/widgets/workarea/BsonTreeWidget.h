#pragma once

#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class BsonTreeWidget : public QTreeWidget
    {
        Q_OBJECT

    public:
        BsonTreeWidget(QWidget *parent = NULL);
        ~BsonTreeWidget();

        void setDocuments(const QList<MongoDocumentPtr> &documents);
        QIcon getIcon(MongoElementPtr element);

    public slots:
        void ui_itemExpanded(QTreeWidgetItem *item);

    protected slots:
        void resizeEvent(QResizeEvent *event);

    private:
        QList<MongoDocumentPtr> _documents;
    };
}
