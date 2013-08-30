#include "robomongo/gui/widgets/workarea/BsonTableItem.h"

#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoDocumentIterator.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace mongo;

namespace
{
    QString buildSynopsis(const QString &text)
    {
        QString simplified = text.simplified().left(300);
        return simplified;
    }
}

namespace Robomongo
{
    BsonTableItem::BsonTableItem(MongoDocumentPtr rootDocument, MongoElementPtr element) 
        :_element(element),
        _rootDocument(rootDocument)
    {
       
    }

    mongo::BSONType BsonTableItem::type() const
    {
        return _element->bsonElement().type();
    }

    BsonTableItem::BsonTableItem(MongoDocumentPtr document) 
        :_document(document),
        _rootDocument(document)
    {
    }

    unsigned BsonTableItem::columnCount() const
    {
        return _rows.size();
    }

    unsigned BsonTableItem::childrenCount() const
    {
        return _items.size();
    }

    void BsonTableItem::clear()
    {
        _items.clear();
    }

    void BsonTableItem::addChild(BsonTableItem *item)
    {
        _items.push_back(item);
    }

    BsonTableItem* BsonTableItem::child(unsigned pos)const
    {
        return _items[pos];
    }
}
