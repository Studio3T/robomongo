#pragma once

#include <QStyle>
#include <QProxyStyle>
typedef QProxyStyle OsStyle;

namespace Robomongo
{
    namespace detail
    {
        void applyStyle(const QString &styleName);
        QStringList getSupportedStyles();
        void initStyle();
    }

    class AppStyle 
        :public OsStyle
    {
        Q_OBJECT
    public:
        static const QString StyleName;
        virtual void drawControl(ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget) const;
        virtual void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const;
        virtual QRect subElementRect( SubElement element, const QStyleOption * option, const QWidget * widget=0 ) const;
    };
}
