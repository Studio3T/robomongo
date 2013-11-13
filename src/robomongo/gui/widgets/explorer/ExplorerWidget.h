#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
QT_END_NAMESPACE

#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class MongoServer;
    /**
     * @brief Explorer widget (usually you'll see it at the left of main window)
     */
    class ExplorerWidget : public QWidget
    {
        Q_OBJECT

    public:
        typedef QWidget BaseClass;
        ExplorerWidget(QWidget *parent);

    public Q_SLOTS:
        void addServer(MongoServer *server);
        void decreaseProgress();
        void increaseProgress();

    private Q_SLOTS:
        void ui_itemExpanded(QTreeWidgetItem *item);
        void ui_itemDoubleClicked(QTreeWidgetItem *item, int column);

    protected:
        virtual void keyPressEvent(QKeyEvent *event);   

    private:
        int _progress;
        QLabel *_progressLabel;
        QTreeWidget *_treeWidget;
    };
}
