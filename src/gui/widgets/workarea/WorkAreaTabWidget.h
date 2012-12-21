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

    protected:

        /**
         * @brief Overrides QTabWidget::keyPressEvent() in order to intercept
         * tab close key shortcuts (Ctrl+F4 and Ctrl+W)
         */
        void keyPressEvent(QKeyEvent *event);

    public slots:

        void ui_tabCloseRequested(int index);
        void ui_newTabRequested(int index);
        void ui_currentChanged(int index);
    };
}

#endif // WORKAREATABWIDGET_H
