#pragma once

#include <QStyle>
#include <QProxyStyle>

namespace Robomongo
{
    namespace AppStyleUtils
    {
        void initStyle();
        void applyStyle(const QString &styleName);
        QStringList getSupportedStyles();
    }

    class AppStyle : public QProxyStyle
    {
        Q_OBJECT

    public:
        static const QString StyleName;
        virtual void drawControl(ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget) const;
        virtual void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const;
        virtual QRect subElementRect( SubElement element, const QStyleOption * option, const QWidget * widget = 0 ) const;
    };
}
