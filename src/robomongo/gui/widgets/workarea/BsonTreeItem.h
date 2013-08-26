#pragma once

#include <QObject>
#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    /**
     * @brief BSON tree item (represents array or object)
     */
    class BsonTreeItem : public QTreeWidgetItem
    {
    public:
        typedef QTreeWidgetItem baseClass;
        BsonTreeItem(MongoDocumentPtr rootDocument, MongoElementPtr element, int position);
        BsonTreeItem(MongoDocumentPtr document, int position);

        MongoElementPtr element() const { return _element; }
        MongoDocumentPtr rootDocument() const { return _rootDocument; }
        bool isSimpleType() const;
        bool isUuidType() const;
        void expand();

    private:
        void setupDocument(MongoDocumentPtr document);
        QString buildObjectFieldName();
        QString buildFieldName();
        QString buildArrayFieldName(int itemsCount);

        const MongoElementPtr _element;
        const MongoDocumentPtr _document;
        const MongoDocumentPtr _rootDocument;
        /**
         * @brief Position in array. -1 if not in array
         */
        const int _position;
    };
}
