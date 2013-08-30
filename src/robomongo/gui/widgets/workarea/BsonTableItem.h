#pragma once

#include <QObject>
#include <QTreeWidget>

#include "robomongo/core/Core.h"
#include "mongo/bson/bsontypes.h"

namespace Robomongo
{
    /**
     * @brief BSON tree item (represents array or object)
     */
    class BsonTableItem
    {
    public:
        typedef std::vector<BsonTableItem*> childContainerType;
        typedef std::vector<QString> rowValuesType;
        BsonTableItem(MongoDocumentPtr rootDocument, MongoElementPtr element);
        BsonTableItem(MongoDocumentPtr document);

        MongoElementPtr element() const { return _element; }

        unsigned columnCount() const;
        unsigned childrenCount() const;
        mongo::BSONType type() const;
        void clear();
        void addChild(BsonTableItem *item);
        BsonTableItem* child(unsigned pos)const;

    private:

        const MongoElementPtr _element;
        const MongoDocumentPtr _document;
        const MongoDocumentPtr _rootDocument;

        rowValuesType _rows;
        childContainerType _items;
    };
}
