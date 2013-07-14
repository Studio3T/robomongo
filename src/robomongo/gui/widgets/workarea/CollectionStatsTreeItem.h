#pragma once

#include <QTreeWidgetItem>
#include <mongo/bson/bsonobj.h>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class CollectionStatsTreeItem : public QTreeWidgetItem
    {
    public:
        CollectionStatsTreeItem(MongoDocumentPtr document);
    private:

        QString getString(const char *name) const;
        int getInt(const char *name) const;
        long long getLongLong(const char *name) const;
        double getDouble(const char *name) const;

        MongoDocumentPtr _document;
        mongo::BSONObj _obj;
    };
}
