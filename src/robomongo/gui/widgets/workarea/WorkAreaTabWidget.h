#pragma once

#include <QTabWidget>

namespace Robomongo
{
    class QueryWidget;
    class OpeningShellEvent;
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
        explicit WorkAreaTabWidget(QWidget *parent=0);

        void closeTab(int index);
        void nextTab();
        void previousTab();

        QueryWidget *currentQueryWidget();
        QueryWidget *queryWidget(int index);

    protected:
        /**
         * @brief Overrides QTabWidget::keyPressEvent() in order to intercept
         * tab close key shortcuts (Ctrl+F4 and Ctrl+W)
         */
        virtual void keyPressEvent(QKeyEvent *event);

    public Q_SLOTS:
        void handle(OpeningShellEvent *event);
        void tabBar_tabCloseRequested(int index);
        void ui_newTabRequested(int index);
        void ui_reloadTabRequested(int index);
        void ui_duplicateTabRequested(int index);
        void ui_closeOtherTabsRequested(int index);
        void ui_closeTabsToTheRightRequested(int index);
        void ui_currentChanged(int index);

        void tabTextChange(const QString &text);
        void tooltipTextChange(const QString &text);
    };
}
