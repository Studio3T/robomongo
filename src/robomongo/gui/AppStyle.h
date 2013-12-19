#pragma once

#include <QStyle>
#ifdef OS_WIN
    #include <QProxyStyle>
    typedef QProxyStyle OsStyle;
#elif defined(OS_MAC)
    #include <QProxyStyle>
    typedef QProxyStyle OsStyle;
#elif defined OS_LINUX
    #if !defined(QT_NO_STYLE_GTK)
        #include <QProxyStyle>
        typedef QProxyStyle OsStyle;
    #else
        #include <QCleanlooksStyle>
        typedef QCleanlooksStyle OsStyle;
    #endif
#endif

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
