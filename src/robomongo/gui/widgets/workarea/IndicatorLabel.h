#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QIcon;
class QLabel;
QT_END_NAMESPACE

namespace Robomongo
{
    class Indicator : public QWidget
    {
        Q_OBJECT

    public:
        Indicator(const QIcon &icon, const QString &text = QString());
        void setText(const QString &text);

    private:
        QLabel *createLabelWithIcon(const QIcon &icon);
        QLabel *_label;
    };
}
