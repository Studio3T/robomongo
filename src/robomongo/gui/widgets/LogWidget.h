#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QPlainTextEdit;
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
        void addMessage(const QString &message);

    private:        
        QPlainTextEdit *const _logTextEdit;
    };

}
