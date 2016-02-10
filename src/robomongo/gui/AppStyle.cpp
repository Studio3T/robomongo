#include "robomongo/gui/AppStyle.h"

#include <QApplication>
#include <QStyleFactory>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"

namespace Robomongo
{
    const QString AppStyle::StyleName = "Native";

    namespace AppStyleUtils
    {
        void applyStyle(const QString &styleName)
        {
            if (styleName == "Native") {
                QApplication::setStyle(new AppStyle);
                return;
            }

            QApplication::setStyle(QStyleFactory::create(styleName));
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

    void AppStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
    {
        return QProxyStyle::drawControl(element, option, painter, widget);
    }

    void AppStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
    {
#ifdef Q_OS_WIN
        if (element == QStyle::PE_FrameFocusRect)
            return;
#endif

        return QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    QRect AppStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget /*= 0 */) const
    {
        return QProxyStyle::subElementRect(element, option, widget);
    }
}
