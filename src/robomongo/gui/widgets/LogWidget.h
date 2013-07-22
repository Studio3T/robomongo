#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class LogWidget : public QWidget
    {
        Q_OBJECT

    public:
        LogWidget(QWidget* parent = 0);

    public Q_SLOTS:
        void addMessage(const QString &message);
        void handle(SomethingHappened *event);
        void handle(ConnectingEvent *event);
        void handle(OpeningShellEvent *event);

    private:
        QPlainTextEdit *const _logTextEdit;
    };

}
