#pragma once

#include <QTreeWidget>

namespace Robomongo
{
    class ExplorerTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        explicit ExplorerTreeWidget(QWidget *parent = 0);
    protected:
        virtual void contextMenuEvent(QContextMenuEvent *event);
    };
}
