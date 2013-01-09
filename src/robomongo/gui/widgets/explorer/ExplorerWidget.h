#pragma once

#include <QTreeWidget>
#include <QLabel>

#include "robomongo/core/Core.h"
#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class MongoManager;

    /*
    ** Explorer widget (usually you'll see it at the right of main window)
    */
    class ExplorerWidget : public QWidget
    {
        Q_OBJECT

    public:
        /*
        ** Constructs ExplorerWidget
        */
        ExplorerWidget(QWidget *parent);

    protected:
        void keyPressEvent(QKeyEvent *event);

    public slots:

        /*
        ** Add server to tree view
        */
        void removeServer();

        /*
        ** Handle item expanding
        */
        void ui_itemExpanded(QTreeWidgetItem *item);

        /*
        ** Handle item doubleclicking
        */
        void ui_itemDoubleClicked(QTreeWidgetItem *item, int column);

        /**/
        void ui_customContextMenuRequested(QPoint p);
        void ui_itemClicked(QTreeWidgetItem*,int);
        void ui_disonnectActionTriggered();
        void ui_openShellActionTriggered();

    public slots:
        void handle(ConnectingEvent *event);
        void handle(ConnectionEstablishedEvent *event);
        void handle(ConnectionFailedEvent *event);

    private:

        int _progress;
        void increaseProgress();
        void decreaseProgress();
        QLabel *_progressLabel;

        /*
        ** Main tree widget of the explorer
        */
        QTreeWidget *_treeWidget;

        EventBus *_bus;
        App *_app;
    };
}
