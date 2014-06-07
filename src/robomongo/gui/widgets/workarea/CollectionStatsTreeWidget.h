#pragma once

#include <QTreeWidget>
#include <QEvent>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class CollectionStatsTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        typedef QTreeWidget BaseClass;
        CollectionStatsTreeWidget(const std::vector<MongoDocumentPtr> &documents, QWidget *parent = NULL);
        
    protected:
        void changeEvent(QEvent *event);
        
    private:
        void retranslateUI();
    };
}
