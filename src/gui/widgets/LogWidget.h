#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>

namespace Robomongo
{
    class MainWindow;

    /**
     * Log panel
     */
    class LogWidget : public QWidget
    {
        Q_OBJECT

    private:

        /*
        ** Main window this widget belongs to
        */
        MainWindow *_mainWindow;

        /*
        ** Log text box
        */
        QPlainTextEdit *_logTextEdit;

    public:

        /*
        ** Constructs log widget panel for main window
        */
        LogWidget(MainWindow *mainWindow);

    public slots:

        void vm_queryExecuted(const QString &query, const QString &result);
    };

}


#endif // LOGWIDGET_H
