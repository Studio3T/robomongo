#ifndef BSONTREEWIDGET_H
#define BSONTREEWIDGET_H

#include <QTreeWidget>
#include "Core.h"

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
        BsonTreeWidget(QWidget * parent);
        ~BsonTreeWidget();

        /*
        ** Set documents
        */
        void setDocuments(const QList<MongoDocumentPtr> & documents);

        QIcon getIcon(MongoElementPtr element);

    public slots:

        /*
        ** Handle itemExpanded event
        */
        void ui_itemExpanded(QTreeWidgetItem * item);
    };
}



#endif // BSONTREEWIDGET_H
