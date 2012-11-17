#ifndef EXPLORERWIDGET_H
#define EXPLORERWIDGET_H

#include <QTreeWidget>
#include <QWidget>

namespace Robomongo
{
    class ExplorerViewModel;
    class ExplorerServerViewModel;

    /*
    ** Explorer widget (usually you'll see it at the right of main window)
    */
    class ExplorerWidget : public QWidget
    {
        Q_OBJECT

    private:

        /*
        ** Explorer view model
        */
        ExplorerViewModel *_viewModel;

        /*
        ** Main tree widget of the explorer
        */
        QTreeWidget *_treeWidget;

    public:
        /*
        ** Constructs ExplorerWidget
        */
        ExplorerWidget(QWidget *parent);

    public slots:

        /*
        ** Add server to tree view
        */
        void addServer();

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

    };
}



#endif // EXPLORERWIDGET_H
