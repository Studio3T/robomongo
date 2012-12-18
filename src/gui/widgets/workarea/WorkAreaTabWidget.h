#ifndef WORKAREATABWIDGET_H
#define WORKAREATABWIDGET_H

#include <QTabWidget>

namespace Robomongo
{
    class WorkAreaWidget;

    class WorkAreaTabWidget : public QTabWidget
    {
        Q_OBJECT

    private:

    public:

        /*
        ** Constructs tab widget
        */
        WorkAreaTabWidget(WorkAreaWidget * workAreaWidget);

    public slots:

        void ui_tabCloseRequested(int index);
        void ui_currentChanged(int index);
    };
}

#endif // WORKAREATABWIDGET_H
