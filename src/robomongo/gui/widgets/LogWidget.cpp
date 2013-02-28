#include "robomongo/gui/widgets/LogWidget.h"

#include "robomongo/core/EventBus.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/gui/MainWindow.h"

using namespace Robomongo;

LogWidget::LogWidget(MainWindow *mainWindow) : QWidget(mainWindow),
    _bus(AppRegistry::instance().bus())
{
    _mainWindow = mainWindow;

    _logTextEdit = new QPlainTextEdit;
    _logTextEdit->setPlainText("Robomongo 0.6 is ready.");
    //_logTextEdit->setMarginWidth(1, 3); // to hide left gray column

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setMargin(0);
    hlayout->addWidget(_logTextEdit);

    setLayout(hlayout);

    _bus->subscribe(this, SomethingHappened::Type);
    _bus->subscribe(this, ConnectingEvent::Type);
    _bus->subscribe(this, OpeningShellEvent::Type);
}

void LogWidget::addMessage(const QString &message)
{
    _logTextEdit->appendPlainText(message);

    QScrollBar *sb = _logTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void LogWidget::handle(SomethingHappened *event)
{
    addMessage(event->something);
}

void LogWidget::handle(ConnectingEvent *event)
{
    addMessage(QString("Connecting to %1...").arg(event->server->connectionRecord()->getFullAddress()));
}

void LogWidget::handle(OpeningShellEvent *event)
{
    addMessage("Openning shell...");
}
