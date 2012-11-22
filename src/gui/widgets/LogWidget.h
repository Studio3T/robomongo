#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include "mongodb/MongoManager.h"
#include "settings/ConnectionRecord.h"

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

        /**
         * @brief MongoManager
         */
        MongoManager &_mongoManager;

    public:

        /*
        ** Constructs log widget panel for main window
        */
        LogWidget(MainWindow *mainWindow);

    public slots:

        void vm_queryExecuted(const QString &query, const QString &result);

        void onConnecting(const MongoServerPtr &record);
    };

}


#endif // LOGWIDGET_H
