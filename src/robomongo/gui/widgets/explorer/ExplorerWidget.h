#pragma once

#include <QTreeWidget>
#include <QLabel>

#include "robomongo/core/Core.h"
#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class MongoManager;

    /**
     * @brief Explorer widget (usually you'll see it at the left of main window)
     */
    class ExplorerWidget : public QWidget
    {
        Q_OBJECT

    public:
        ExplorerWidget(QWidget *parent);

    protected:
        void keyPressEvent(QKeyEvent *event);

    public slots:
        void ui_itemExpanded(QTreeWidgetItem *item);
        void ui_itemDoubleClicked(QTreeWidgetItem *item, int column);

        void handle(ConnectingEvent *event);
        void handle(ConnectionEstablishedEvent *event);
        void handle(ConnectionFailedEvent *event);

    private:
        int _progress;
        void increaseProgress();
        void decreaseProgress();
        QLabel *_progressLabel;
        QTreeWidget *_treeWidget;
        EventBus *_bus;
        App *_app;
    };
}
