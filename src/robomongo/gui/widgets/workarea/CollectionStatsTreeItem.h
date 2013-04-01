#pragma once

#include <QTreeWidgetItem>
#include "mongo/client/dbclient.h"

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class CollectionStatsTreeItem : public QTreeWidgetItem
    {
    public:
        CollectionStatsTreeItem(MongoDocumentPtr document);
    private:

        QString getString(const char *name);
        int getInt(const char *name);
        long long getLongLong(const char *name);
        double getDouble(const char *name);
        QString value(const QString &data);

        MongoDocumentPtr _document;
        mongo::BSONObj _obj;
    };
}
