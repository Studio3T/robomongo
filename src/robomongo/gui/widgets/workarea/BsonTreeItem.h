#pragma once

#include <QObject>
#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class MongoElement;

    /**
     * @brief BSON tree item (represents array or object)
     */
    class BsonTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT
    public:
        BsonTreeItem(MongoDocumentPtr rootDocument, MongoElementPtr element, int position);
        BsonTreeItem(MongoDocumentPtr document, int position);
        ~BsonTreeItem() {}

        MongoElementPtr element() const { return _element; }
        MongoDocumentPtr rootDocument() const { return _rootDocument; }
        bool isSimpleType();
        bool isUuidType();
        void expand();

    private:
        void setupDocument(MongoDocumentPtr document);
        void cleanChildItems();
        QString buildObjectFieldName();
        QString buildFieldName();
        QString buildArrayFieldName(int itemsCount);

        MongoElementPtr _element;
        MongoDocumentPtr _document;

        MongoDocumentPtr _rootDocument;
        /**
         * @brief Position in array. -1 if not in array
         */
        int _position;
    };
}
