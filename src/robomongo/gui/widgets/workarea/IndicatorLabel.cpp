#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QIcon>

namespace Robomongo
{

    Indicator::Indicator(const QIcon &icon, const QString &text)
    {
        QLabel *iconLabel = createLabelWithIcon(icon);
        _label = new QLabel();

        QHBoxLayout *layout = new QHBoxLayout();
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(iconLabel);
        layout->addSpacing(7);
        layout->addWidget(_label);
        layout->addSpacing(14);
        setLayout(layout);

        setText(text);
    }

    void Indicator::setText(const QString &text)
    {
        _label->setText(text);
    }

    QLabel *Indicator::createLabelWithIcon(const QIcon &icon)
    {
        QPixmap pixmap = icon.pixmap(16, 16);
        QLabel *label = new QLabel;
        label->setPixmap(pixmap);
        return label;
    }
}
