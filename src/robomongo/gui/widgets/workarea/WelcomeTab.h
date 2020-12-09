#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QScrollArea;
QT_END_NAMESPACE

namespace Robomongo
{
    class WelcomeTab : public QWidget
    {
        Q_OBJECT

    public:
        WelcomeTab(QScrollArea *parent = nullptr);
        ~WelcomeTab() {}

        QScrollArea* getParent() const { return _parent; }
        void resize() { /* Not implemented for Windows and macOS */}

    private:
        QScrollArea* _parent;
    };
}