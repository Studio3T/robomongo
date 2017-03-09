#include "robomongo/gui/widgets/workarea/WorkAreaTabBar.h"

#include <QMouseEvent>
#include <QTabWidget>
#include <QScrollArea>

namespace Robomongo
{
    /**
     * @brief Creates WorkAreaTabBar, without parent widget. We are
     * assuming, that tab bar will be installed to (and owned by)
     * WorkAreaTabWidget, using QTabWidget::setTabBar().
     */
    WorkAreaTabBar::WorkAreaTabBar(QWidget *parent) 
        : QTabBar(parent)
    {
        setDrawBase(false);

    #if !defined(Q_OS_MAC)
        setStyleSheet(buildStyleSheet());
    #endif

        _menu = new QMenu(this);

        _newShellAction = new QAction("&New Shell", _menu);
        _newShellAction->setShortcut(QKeySequence(QKeySequence::AddTab));
        _reloadShellAction = new QAction("&Re-execute Query", _menu);
        _reloadShellAction->setShortcut(Qt::CTRL + Qt::Key_R);
        _duplicateShellAction = new QAction("&Duplicate Query In New Tab", _menu);
        _duplicateShellAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_T);
        _pinShellAction = new QAction("&Pin Shell", _menu);
        _closeShellAction = new QAction("&Close Shell", _menu);
        _closeShellAction->setShortcut(Qt::CTRL + Qt::Key_W);
        _closeOtherShellsAction = new QAction("Close &Other Shells", _menu);
        _closeShellsToTheRightAction = new QAction("Close Shells to the R&ight", _menu);

        _menu->addAction(_newShellAction);
        _menu->addSeparator();
        _menu->addAction(_reloadShellAction);
        _menu->addAction(_duplicateShellAction);
        _menu->addSeparator();
        _menu->addAction(_closeShellAction);
        _menu->addAction(_closeOtherShellsAction);
        _menu->addAction(_closeShellsToTheRightAction);
    }

    /**
     * @brief Overrides QTabBar::mouseReleaseEvent() in order to support
     * middle-mouse tab close and to implement tab context menu.
     */
    void WorkAreaTabBar::mouseReleaseEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::MidButton)
            middleMouseReleaseEvent(event);
        else if (event->button() == Qt::RightButton)
            rightMouseReleaseEvent(event);

        // always calling base event handler, even if we
        // were interested by this event
        QTabBar::mouseReleaseEvent(event);
    }

    void WorkAreaTabBar::mouseDoubleClickEvent(QMouseEvent *event)
    {
        int tabIndex = tabAt(event->pos());

        // if tab was double-clicked, ignore this action
        if (tabIndex >= 0)
            return;

        int currentTab = currentIndex();
        if (currentTab < 0)
            return;

        emit newTabRequested(currentTab);
        QTabBar::mouseDoubleClickEvent(event);
    }

    /**
     * @brief Handles middle-mouse release event in order to close tab.
     */
    void WorkAreaTabBar::middleMouseReleaseEvent(QMouseEvent *event)
    {
        int tabIndex = tabAt(event->pos());
        if (tabIndex < 0)
            return;

        emit tabCloseRequested(tabIndex);
    }

    /**
     * @brief Handles right-mouse release event to show tab context menu.
     */
    void WorkAreaTabBar::rightMouseReleaseEvent(QMouseEvent *event)
    {
        int tabIndex = tabAt(event->pos());
        if (tabIndex < 0)
            return;

        // If this is a Welcome tab, do not show right click menu. 
        // Note: Scroll area represents a WelcomeTab.
        auto tabWidget = qobject_cast<QTabWidget*>(parentWidget());
        if (qobject_cast<QScrollArea*>(tabWidget->widget(tabIndex)))
            return;

        QAction *selected = _menu->exec(QCursor::pos());
        if (!selected)
            return;

        emitSignalForContextMenuAction(tabIndex, selected);
    }

    /**
     * @brief Emits signal, based on specified action. Only actions
     * specified in this class are supported. If we don't know specified
     * action - no signal will be emited.
     * @param tabIndex: index of tab, for which signal will be emited.
     * @param action: context menu action.
     */
    void WorkAreaTabBar::emitSignalForContextMenuAction(int tabIndex, QAction *action)
    {
        if (action == _newShellAction)
            emit newTabRequested(tabIndex);
        else if (action == _reloadShellAction)
            emit reloadTabRequested(tabIndex);
        else if (action == _duplicateShellAction)
            emit duplicateTabRequested(tabIndex);
        else if (action == _pinShellAction)
            emit pinTabRequested(tabIndex);
        else if (action == _closeShellAction)
            emit tabCloseRequested(tabIndex);
        else if (action == _closeOtherShellsAction)
            emit closeOtherTabsRequested(tabIndex);
        else if (action == _closeShellsToTheRightAction)
            emit closeTabsToTheRightRequested(tabIndex);
    }

    /**
     * @brief Builds stylesheet for this WorkAreaTabBar widget.
     */
    QString WorkAreaTabBar::buildStyleSheet()
    {
        QColor background = palette().window().color();
        QColor gradientZero = QColor("#ffffff"); //Qt::white;//.lighter(103);
        QColor gradientOne =  background.lighter(104); //Qt::white;//.lighter(103);
        QColor gradientTwo =  background.lighter(108); //.lighter(103);
        QColor selectedBorder = background.darker(103);

        QString aga1 = gradientOne.name();
        QString aga2 = gradientTwo.name();
        QString aga3 = background.name();

        QString styles = QString(
            "QTabBar::tab:first {"
                "margin-left: 4px;"
            "}  "
            "QTabBar::tab:last {"
                "margin-right: 1px;"
            "}  "
            "QTabBar::close-button { "
                "image: url(:/robomongo/icons/close_2_16x16.png);"
                "width: 10px;"
                "height: 10px;"
            "}"
            "QTabBar::close-button:hover { "
                  "image: url(:/robomongo/icons/close_hover_16x16.png);"
                  "width: 15px;"
                  "height: 15px;"
            "}"
            "QTabBar::tab {"
                "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                                            "stop: 0 #F0F0F0, stop: 0.4 #DEDEDE,"
                                            "stop: 0.5 #E6E6E6, stop: 1.0 #E1E1E1);"
                "border: 1px solid #C4C4C3;"
                "border-bottom-color: #B8B7B6;" // #C2C7CB same as the pane color
                "border-top-left-radius: 6px;"
                "border-top-right-radius: 6px;"
    //            "min-width: 8ex;"
                "max-width: 200px;"
                "padding: 4px 0px 5px 0px;"
                "margin: 0px;"
                "margin-left: 1px;"
                "margin-right: -3px;"  // it should be -(tab:first:margin-left + tab:last:margin-left) to fix incorrect text elidement
            "}"

            "QTabBar::tab:selected, QTabBar::tab:hover {"
                "/* background: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0,"
                                            "stop: 0 %1, stop: 0.3 %2,"    //#fafafa, #f4f4f4
                                            "stop: 0.6 %3, stop: 1.0 %4); */" //#e7e7e7, #fafafa
                "background-color: white;"
            "}"

            "QTabBar::tab:selected {"
                "border-color: #9B9B9B;" //
                "border-bottom-color: %4;" //#fafafa
            "}"

            "QTabBar::tab:!selected {"
                "margin-top: 2px;" // make non-selected tabs look smaller
            "}  "
            "QTabBar::tab:only-one { margin-top: 2px; margin-left:4px; }"
        ).arg(gradientZero.name(), gradientOne.name(), gradientTwo.name(), "#ffffff");

        QString aga = palette().window().color().name();

        return styles;
    }
}
