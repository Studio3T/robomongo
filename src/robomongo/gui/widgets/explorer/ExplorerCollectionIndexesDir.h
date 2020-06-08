#pragma once

#include "robomongo/gui/widgets/explorer/ExplorerTreeItem.h"

namespace Robomongo
{
    class ExplorerCollectionIndexesDir : public ExplorerTreeItem
    {
        Q_OBJECT
    public:
        using BaseClass = ExplorerTreeItem;
        static const QString labelText;
        explicit ExplorerCollectionIndexesDir(QTreeWidgetItem *parent);
        void expand();

    private Q_SLOTS:
        void ui_addIndex();
        void ui_reIndex();
        void ui_viewIndex();
        void ui_refreshIndex();
    };

}