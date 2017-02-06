#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace Robomongo
{

    class WelcomeTab : public QWidget
    {
        Q_OBJECT

    public:
        WelcomeTab(QWidget *parent = nullptr);
        ~WelcomeTab();

    };

}
