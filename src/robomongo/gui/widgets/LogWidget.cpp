#include "robomongo/gui/widgets/LogWidget.h"

#include <QHBoxLayout>
#include <QScrollBar>
#include <QPlainTextEdit>

#include "robomongo/core/EventBus.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"

namespace Robomongo
{
    LogWidget::LogWidget(QWidget* parent) : QWidget(parent)
    {
        _logTextEdit = new QPlainTextEdit(this);
        _logTextEdit->setPlainText(PROJECT_NAME " " PROJECT_VERSION " is ready.");
        //_logTextEdit->setMarginWidth(1, 3); // to hide left gray column

        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setMargin(0);
        hlayout->addWidget(_logTextEdit);

        setLayout(hlayout);

        AppRegistry::instance().bus()->subscribe(this, SomethingHappened::Type);
        AppRegistry::instance().bus()->subscribe(this, ConnectingEvent::Type);
        AppRegistry::instance().bus()->subscribe(this, OpeningShellEvent::Type);
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
}
