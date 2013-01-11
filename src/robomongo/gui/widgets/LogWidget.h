#pragma once

#include <QWidget>
#include <QPlainTextEdit>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/settings/ConnectionSettings.h"

namespace Robomongo
{
    class MainWindow;

    class LogWidget : public QWidget
    {
        Q_OBJECT

    public:
        LogWidget(MainWindow *mainWindow);

    public slots:
        void addMessage(const QString &message);
        void handle(SomethingHappened *event);
        void handle(ConnectingEvent *event);
        void handle(OpeningShellEvent *event);

    private:
        /**
         * @brief Main window this widget belongs to
         */
        MainWindow *_mainWindow;

        QPlainTextEdit *_logTextEdit;
        EventBus *_bus;
    };

}
