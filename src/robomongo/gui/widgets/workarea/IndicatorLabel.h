#pragma once

#include <QWidget>
#include <QIcon>
#include <QLabel>

namespace Robomongo
{
    class OutputItemContentWidget;
    class OutputItemWidget;
    class OutputWidget;

    class Indicator : public QWidget
    {
        Q_OBJECT
    public:
        Indicator(const QIcon &icon);
        void setText(const QString &text);
    private:
        QLabel *createLabelWithIcon(const QIcon &icon);
        QLabel *_label;
    };
}
