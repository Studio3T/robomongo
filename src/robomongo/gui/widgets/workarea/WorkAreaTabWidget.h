#pragma once

#include <QTabWidget>

namespace Robomongo
{
    class QueryWidget;
    class WorkAreaWidget;
    class EventBus;

    /**
     * @brief WorkArea tab widget. Each tab represents MongoDB shell.
     */
    class WorkAreaTabWidget : public QTabWidget
    {
        Q_OBJECT

    public:
        /**
         * @brief Creates WorkAreaTabWidget.
         * @param workAreaWidget: WorkAreaWidget this tab belongs to.
         */
        WorkAreaTabWidget(WorkAreaWidget *workAreaWidget);

        void closeTab(int index);

        QueryWidget *currentQueryWidget();
        QueryWidget *queryWidget(int index);

    protected:
        /**
         * @brief Overrides QTabWidget::keyPressEvent() in order to intercept
         * tab close key shortcuts (Ctrl+F4 and Ctrl+W)
         */
        void keyPressEvent(QKeyEvent *event);

    public slots:
        void tabBar_tabCloseRequested(int index);
        void ui_newTabRequested(int index);
        void ui_reloadTabRequested(int index);
        void ui_duplicateTabRequested(int index);
        void ui_closeOtherTabsRequested(int index);
        void ui_closeTabsToTheRightRequested(int index);
        void ui_currentChanged(int index);

    private:

        EventBus *_bus;
    };
}
