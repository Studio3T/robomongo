#pragma once


#include <QWidget>
#include <mongo/logger/log_severity.h>
QT_BEGIN_NAMESPACE
class QTextEdit;
class QAction;
QT_END_NAMESPACE

namespace Robomongo
{
    class LogWidget : public QWidget
    {
        Q_OBJECT

    public:
        typedef QWidget BaseClass;
        LogWidget(QWidget* parent = 0);        

    public Q_SLOTS:
        void addMessage(const QString &message, mongo::logger::LogSeverity level);

    private Q_SLOTS:
        void showContextMenu(const QPoint &pt);

    private:        
        QTextEdit *const _logTextEdit;
        QAction *_clear;
    };

}
