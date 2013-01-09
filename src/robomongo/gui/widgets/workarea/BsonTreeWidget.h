#pragma once

#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class BsonTreeWidget : public QTreeWidget
    {
        Q_OBJECT

    private:

        /*
        ** Current set of documents
        */
        QList<MongoDocumentPtr> _documents;

    public:

        /*
        ** Constructs Bson Tree widget
        */
        BsonTreeWidget(QWidget * parent = NULL);
        ~BsonTreeWidget();

        /*
        ** Set documents
        */
        void setDocuments(const QList<MongoDocumentPtr> & documents);

        QIcon getIcon(MongoElementPtr element);

        void resizeEvent(QResizeEvent *event);

    public slots:

        /*
        ** Handle itemExpanded event
        */
        void ui_itemExpanded(QTreeWidgetItem * item);
    };
}
