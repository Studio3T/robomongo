#pragma once

#include <QObject>
#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class MongoElement;

    /*
    ** BSON tree item (represents array or object)
    */
    class BsonTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    public:
        /*
        ** Constructs BsonTreeItem
        */
        BsonTreeItem(MongoElementPtr element, int position);
        ~BsonTreeItem();

        /*
        ** Constructs BsonTreeItem
        */
        BsonTreeItem(MongoDocumentPtr document, int position);

        /*
        ** MongoDocument this tree item represents
        */
        MongoElementPtr element() const { return _element; }

        void expand();

    private:
        /*
        ** MongoDocument this tree item represents
        */
        MongoElementPtr _element;

        /*
        ** Document
        */
        MongoDocumentPtr _document;

        /*
        ** Position in array. -1 if not in array
        */
        int _position;

        /*
        ** Setup item that represents bson document
        */
        void setupDocument(MongoDocumentPtr document);

        /*
        ** Clean child items
        */
        void cleanChildItems();

        QString buildObjectFieldName();
        QString buildFieldName();
        QString buildArrayFieldName(int itemsCount);
        QString buildSynopsis(QString text);
    };
}
