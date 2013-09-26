#include "robomongo/gui/widgets/LogWidget.h"

#include <QHBoxLayout>
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QPlainTextEdit>
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    LogWidget::LogWidget(QWidget* parent) 
        : BaseClass(parent), _logTextEdit(new QPlainTextEdit(this))
    {
        _logTextEdit->setReadOnly(true);
        _logTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
        VERIFY(connect(_logTextEdit,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(showContextMenu(const QPoint &))));
        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setContentsMargins(0,0,0,0);
        hlayout->addWidget(_logTextEdit);
        _clear = new QAction("Clear All", this);
        VERIFY(connect(_clear, SIGNAL(triggered()),_logTextEdit, SLOT(clear())));
        setLayout(hlayout);      
    }

    void LogWidget::showContextMenu(const QPoint &pt)
    {
        QMenu *menu = _logTextEdit->createStandardContextMenu();
        menu->addAction(_clear);
        _clear->setEnabled(!_logTextEdit->toPlainText().isEmpty());

        menu->exec(_logTextEdit->mapToGlobal(pt));
        delete menu;
    }

    void LogWidget::addMessage(const QString &message, mongo::LogLevel level)
    {
        if (level == mongo::LL_ERROR) {
            _logTextEdit->appendHtml(QString("<font color=red>%1</font>").arg(message));
        }
        else {
            _logTextEdit->appendHtml(QString("<font color=black>%1</font>").arg(message));
        }
        QScrollBar *sb = _logTextEdit->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}
