#include "LogWidget.h"
#include "MainWindow.h"
#include "Dispatcher.h"
#include "AppRegistry.h"
#include "mongodb/MongoServer.h"

using namespace Robomongo;

/*
** Constructs log widget panel for main window
*/
LogWidget::LogWidget(MainWindow *mainWindow) : QWidget(mainWindow),
    _mongoManager(AppRegistry::instance().mongoManager())
{
    connect(&_mongoManager, SIGNAL(connecting(MongoServerPtr)), this, SLOT(onConnecting(MongoServerPtr)));

    _mainWindow = mainWindow;

    _logTextEdit = new QPlainTextEdit;
    _logTextEdit->setPlainText("Robomongo 0.3 is ready.");
    //_logTextEdit->setMarginWidth(1, 3); // to hide left gray column

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setMargin(0);
    hlayout->addWidget(_logTextEdit);

    setLayout(hlayout);

    AppRegistry::instance().dispatcher().subscribe(this, SomethingHappened::EventType);
}

bool LogWidget::event(QEvent * event)
{
    R_HANDLE(event) {
        R_EVENT(SomethingHappened);
    }

    QObject::event(event);
}

void LogWidget::vm_queryExecuted(const QString &query, const QString &result)
{
//    _logTextEdit->append(query);
//    _logTextEdit->append(result);

    // Scroll to bottom
    QScrollBar *sb = _logTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void LogWidget::onConnecting(const MongoServerPtr &record)
{
    _logTextEdit->appendPlainText(QString("Connecting to %1...").arg(record->connectionRecord()->getFullAddress()));

    QScrollBar *sb = _logTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void LogWidget::handle(const SomethingHappened *event)
{
    _logTextEdit->appendPlainText(event->something);

    QScrollBar *sb = _logTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}
