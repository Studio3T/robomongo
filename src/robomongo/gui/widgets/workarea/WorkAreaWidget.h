#pragma once

#include <QWidget>

namespace Robomongo
{
    class WorkAreaTabWidget;

    /**
     * The only reason we are using WorkAreaWidget, is to set
     * top padding for inner tab widget (WorkAreaTabWidget)
     */
    class WorkAreaWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit WorkAreaWidget(WorkAreaTabWidget *tabWidget);

    private:
        WorkAreaTabWidget *_tabWidget;
    };
}
