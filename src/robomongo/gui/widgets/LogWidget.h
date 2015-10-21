#pragma once

#ifndef MONGO_UTIL_LOG_H_
#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kStorage

#include <mongo/logger/log_severity.h>
#include <mongo/util/log.h>
#endif

#include <QWidget>
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
        void addMessage(const QString &message, ::mongo::logger::LogSeverity level);

    private Q_SLOTS:
        void showContextMenu(const QPoint &pt);

    private:
        QTextEdit *const _logTextEdit;
        QAction *_clear;
    };

}
