#include "LogWidget.h"
#include "MainWindow.h"
#include "Dispatcher.h"
#include "AppRegistry.h"
#include "domain/MongoServer.h"

using namespace Robomongo;

/*
** Constructs log widget panel for main window
*/
LogWidget::LogWidget(MainWindow *mainWindow) : QWidget(mainWindow),
    _dispatcher(AppRegistry::instance().dispatcher())
{
    _mainWindow = mainWindow;

    _logTextEdit = new QPlainTextEdit;
    _logTextEdit->setPlainText("Robomongo 0.3 is ready.");
    //_logTextEdit->setMarginWidth(1, 3); // to hide left gray column

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setMargin(0);
    hlayout->addWidget(_logTextEdit);

    setLayout(hlayout);

    _dispatcher.subscribe(this, SomethingHappened::EventType);
    _dispatcher.subscribe(this, ConnectingEvent::EventType);
}

bool LogWidget::event(QEvent * event)
{
    R_HANDLE(event) {
        R_EVENT(SomethingHappened);
        R_EVENT(ConnectingEvent);
    }

    QObject::event(event);
}

void LogWidget::addMessage(const QString &message)
{
    _logTextEdit->appendPlainText(message);

    QScrollBar *sb = _logTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void LogWidget::handle(const SomethingHappened *event)
{
    addMessage(event->something);
}

void LogWidget::handle(const ConnectingEvent *event)
{
    addMessage(QString("Connecting to %1...").arg(event->server->connectionRecord()->getFullAddress()));
}
