#include "LogWidget.h"
#include "MainWindow.h"

using namespace Robomongo;

/*
** Constructs log widget panel for main window
*/
LogWidget::LogWidget(MainWindow *mainWindow) : QWidget(mainWindow)
{
    _mainWindow = mainWindow;

    _logTextEdit = new QPlainTextEdit;
    _logTextEdit->setPlainText("Robomongo 0.3 is ready.");
    //_logTextEdit->setMarginWidth(1, 3); // to hide left gray column

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setMargin(0);
    hlayout->addWidget(_logTextEdit);

    setLayout(hlayout);
}

void LogWidget::vm_queryExecuted(const QString &query, const QString &result)
{
//    _logTextEdit->append(query);
//    _logTextEdit->append(result);

    // Scroll to bottom
    QScrollBar *sb = _logTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}
