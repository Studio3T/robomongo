#include "robomongo/gui/widgets/workarea/CollectionStatsTreeItem.h"

#include "mongo/client/dbclient.h"

#include "robomongo/core/domain/MongoUtils.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoNamespace.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

CollectionStatsTreeItem::CollectionStatsTreeItem(MongoDocumentPtr document) :
    _document(document)
{
    _obj = document->bsonObj();

    MongoNamespace ns(getString("ns"));

    setText(0, value(ns.collectionName()));
    setIcon(0, GuiRegistry::instance().collectionIcon());
    setText(1, value(QString::number(getInt("count"))));
    setText(2, value(MongoUtils::buildNiceSizeString(getInt("size"))));
    setText(3, value(MongoUtils::buildNiceSizeString(getInt("storageSize"))));
    setText(4, value(MongoUtils::buildNiceSizeString(getInt("totalIndexSize"))));
    setText(5, value(MongoUtils::buildNiceSizeString(getDouble("avgObjSize"))));
    setText(6, value(QString::number(getDouble("paddingFactor"))));
}

QString CollectionStatsTreeItem::getString(const char *name)
{
    const char * chars = _obj.getStringField(name);
    QString value = QString::fromUtf8(chars);
    return value;
}

int CollectionStatsTreeItem::getInt(const char *name)
{
    return _obj.getIntField(name);
}

double CollectionStatsTreeItem::getDouble(const char *name)
{
    return _obj.getField(name).numberDouble();
}

QString CollectionStatsTreeItem::value(const QString &data)
{
    return data + "     "; // ugly yet simple way to extend size of columns
}
