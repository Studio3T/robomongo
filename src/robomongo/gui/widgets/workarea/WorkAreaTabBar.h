#pragma once

#include <QTabBar>
#include <QMenu>

namespace Robomongo
{
    /**
     * @brief Tab bar for WorkAreaTabWidget.
     */
    class WorkAreaTabBar : public QTabBar
    {
        Q_OBJECT

    public:
        /**
         * @brief Creates WorkAreaTabBar, without parent widget. We are
         * assuming, that tab bar will be installed to (and owned by)
         * WorkAreaTabWidget, using QTabWidget::setTabBar().
         */
        explicit WorkAreaTabBar(QWidget* parent = 0);

    Q_SIGNALS:
        /**
         * @brief Emitted when user requests new tab creation.
         * @param tabIndex: index of tab on which context menu was called.
         */
        void newTabRequested(int tabIndex);

        /**
         * @brief Emitted when user requests tab reload.
         * @param tabIndex: index of tab on which context menu was called.
         */
        void reloadTabRequested(int tabIndex);

        /**
         * @brief Emitted when user requests tab duplication.
         * @param tabIndex: index of tab on which context menu was called.
         */
        void duplicateTabRequested(int tabIndex);

        /**
         * @brief Emitted when user requests tab "pinning".
         * @param tabIndex: index of tab on which context menu was called.
         */
        void pinTabRequested(int tabIndex);

        /**
         * @brief Emitted when user requests to close all other tabs.
         * @param tabIndex: index of tab, that should be left opened.
         */
        void closeOtherTabsRequested(int tabIndex);

        /**
         * @brief Emitted when user requests to close all tabs to the right
         * of tab with 'tabIndex' index
         * @param tabIndex: index of tab on which context menu was called.
         */
        void closeTabsToTheRightRequested(int tabIndex);
    protected:
        /**
         * @brief Overrides QTabBar::mouseReleaseEvent() in order to support
         * middle-mouse tab close and to implement tab context menu.
         */
        void mouseReleaseEvent(QMouseEvent *event);

        /**
         * @brief Overrides QTabBar::mouseDoubleClickEvent() in order to
         * open new shell.
         */
        void mouseDoubleClickEvent(QMouseEvent *);

    private:
        /**
         * @brief Handles middle-mouse release event in order to close tab.
         */
        void middleMouseReleaseEvent(QMouseEvent *event);

        /**
         * @brief Handles right-mouse release event to show tab context menu.
         */
        void rightMouseReleaseEvent(QMouseEvent *event);

        /**
         * @brief Emits signal, based on specified action. Only actions
         * specified in this class are supported. If we don't know specified
         * action - no signal will be emited.
         * @param tabIndex: index of tab, for which signal will be emited.
         * @param action: context menu action.
         */
        void emitSignalForContextMenuAction(int tabIndex, QAction *action);

        /**
         * @brief Builds stylesheet for this WorkAreaTabBar widget.
         */
        QString buildStyleSheet();

        /**
         * @brief Tab's context menu.
         */
        QMenu *_menu;

        /**
         * @brief Tab's context menu actions.
         */
        QAction *_newShellAction;
        QAction *_reloadShellAction;
        QAction *_duplicateShellAction;
        QAction *_pinShellAction;
        QAction *_closeShellAction;
        QAction *_closeOtherShellsAction;
        QAction *_closeShellsToTheRightAction;
    };
}
