#include "robomongo/gui/AppStyle.h"

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include <QApplication>
#include <QStyleFactory>

namespace Robomongo
{
    const QString AppStyle::StyleName = "Native";

    namespace detail
    {
        void applyStyle(const QString &styleName)
        {
            if (styleName == "Native") {
                QApplication::setStyle(new AppStyle);
            }
            else {
                QApplication::setStyle(QStyleFactory::create(styleName));
            }
        }

        QStringList getSupportedStyles()
        {
            static QStringList result = QStringList() << AppStyle::StyleName << QStyleFactory::keys();
            return result;
        }

        void initStyle()
        {
            AppRegistry::instance().settingsManager()->save();
            QString style = AppRegistry::instance().settingsManager()->currentStyle();
            applyStyle(style);
        }
    }

    void AppStyle::drawControl(ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget) const
    {
        return OsStyle::drawControl(element, option, painter, widget);
    }

    void AppStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
    {
#ifdef Q_OS_WIN

        if(element==QStyle::PE_FrameFocusRect)
            return;

#endif // Q_OS_WIN

        return OsStyle::drawPrimitive(element, option, painter, widget);
    }

    QRect AppStyle::subElementRect( SubElement element, const QStyleOption * option, const QWidget * widget /*= 0 */ ) const
    {
        return OsStyle::subElementRect(element, option, widget);
    }
}
