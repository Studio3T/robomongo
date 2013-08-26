#pragma once

#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class MongoShell;

    class CollectionStatsTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        CollectionStatsTreeWidget(MongoShell *shell, QWidget *parent = NULL);
        void setDocuments(const std::vector<MongoDocumentPtr> &documents);
    };
}
