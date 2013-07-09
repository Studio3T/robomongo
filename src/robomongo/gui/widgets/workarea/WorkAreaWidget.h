#pragma once
#include <QWidget>

namespace Robomongo
{
    class WorkAreaTabWidget;
    class EventBus;
    class OpeningShellEvent;
    class MainWindow;
    class QueryWidget;
    /*
    ** Work Area widget
    */
    class WorkAreaWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit WorkAreaWidget(MainWindow *mainWindow);

        void toggleOrientation();
        void executeScript();
        void stopScript();
        void enterTextMode();
        void enterTreeMode();
        void enterCustomMode();
        int countTab()const;
        QueryWidget *const currentWidget() const;
    Q_SIGNALS:
        void tabActivated(int tab);
    public Q_SLOTS:
        void handle(OpeningShellEvent *event);

    private:

        WorkAreaTabWidget *_tabWidget;
        EventBus *_bus;
    };
}
