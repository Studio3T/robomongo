#ifndef EXPLORERTREEWIDGET_H
#define EXPLORERTREEWIDGET_H

#include <QTreeWidget>

namespace Robomongo
{
    class ExplorerTreeWidget : public QTreeWidget
    {
        Q_OBJECT

    private:

        /*
        ** Server context menu
        */
        QMenu *_serverMenu;

    public:

        explicit ExplorerTreeWidget(QWidget *parent = 0);

    protected:

        /*
        ** Virtual method that used to handle rights clicks
        */
        void contextMenuEvent(QContextMenuEvent *event);

    signals:

        void disconnectActionTriggered();
        void refreshActionTriggered();

    protected slots:

        void ui_disconnectServer();
        void ui_refreshServer();

    };
}

#endif // EXPLORERTREEWIDGET_H
