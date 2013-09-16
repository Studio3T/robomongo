#pragma once

#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class CollectionStatsTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        CollectionStatsTreeWidget(QWidget *parent = NULL);
        void setDocuments(const std::vector<MongoDocumentPtr> &documents);
    };
}
