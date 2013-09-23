#pragma once

#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class CollectionStatsTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        CollectionStatsTreeWidget(const std::vector<MongoDocumentPtr> &documents, QWidget *parent = NULL);
    };
}
