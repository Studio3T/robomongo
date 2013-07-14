#include "robomongo/gui/widgets/workarea/CollectionStatsTreeItem.h"

#include "robomongo/core/domain/MongoUtils.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoNamespace.h"
#include "robomongo/gui/GuiRegistry.h"

namespace 
{
    QString prepareValue(const QString &data)
    {
        return data + "     "; // ugly yet simple way to extend size of columns
    }
}
namespace Robomongo
{

    CollectionStatsTreeItem::CollectionStatsTreeItem(MongoDocumentPtr document) :
    _document(document)
    {
        _obj = document->bsonObj();

        MongoNamespace ns(getString("ns"));

        setText(0, prepareValue(ns.collectionName()));
        setIcon(0, GuiRegistry::instance().collectionIcon());
        setText(1, prepareValue(QString::number(getLongLong("count"))));
        setText(2, prepareValue(MongoUtils::buildNiceSizeString(getDouble("size"))));
        setText(3, prepareValue(MongoUtils::buildNiceSizeString(getDouble("storageSize"))));
        setText(4, prepareValue(MongoUtils::buildNiceSizeString(getDouble("totalIndexSize"))));
        setText(5, prepareValue(MongoUtils::buildNiceSizeString(getDouble("avgObjSize"))));
        setText(6, prepareValue(QString::number(getDouble("paddingFactor"))));
    }

    QString CollectionStatsTreeItem::getString(const char *name) const
    {
        const char * chars = _obj.getStringField(name);
        QString value = QString::fromUtf8(chars);
        return value;
    }

    int CollectionStatsTreeItem::getInt(const char *name) const
    {
        return _obj.getIntField(name);
    }

    long long CollectionStatsTreeItem::getLongLong(const char *name) const
    {
        return _obj.getField(name).safeNumberLong();
    }

    double CollectionStatsTreeItem::getDouble(const char *name) const
    {
        return _obj.getField(name).numberDouble();
    }
}
