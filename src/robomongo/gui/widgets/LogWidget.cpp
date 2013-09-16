#include "robomongo/gui/widgets/LogWidget.h"

#include <QHBoxLayout>
#include <QScrollBar>
#include <QPlainTextEdit>
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    LogWidget::LogWidget(QWidget* parent) 
        : BaseClass(parent), _logTextEdit(new QPlainTextEdit(this))
    {
        _logTextEdit->setPlainText(PROJECT_NAME " " PROJECT_VERSION " is ready.");
        //_logTextEdit->setMarginWidth(1, 3); // to hide left gray column
        _logTextEdit->setReadOnly(true);
        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setMargin(0);
        hlayout->addWidget(_logTextEdit);

        setLayout(hlayout);      
    }

    void LogWidget::addMessage(const QString &message)
    {
        _logTextEdit->appendPlainText(message);
        QScrollBar *sb = _logTextEdit->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}
