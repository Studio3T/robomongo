#include "robomongo/gui/utils/DialogUtils.h"

namespace Robomongo
{
    namespace utils
    {
        namespace
        {
            //: Title for dialogs
            const char* titleTemplate = QT_TRANSLATE_NOOP("Robomongo::DialogUtils", "%1 %2");
            //: Text for dialogs
            const char* textTemplate = QT_TRANSLATE_NOOP("Robomongo::DialogUtils", "%1 <b>%3</b> %2?");
        }

        int questionDialog(QWidget *parent, const QString &actionText, const QString &itemText, const QString& valueText)
        {
            return questionDialog(parent, actionText, itemText, QApplication::translate("Robomongo::DialogUtils", textTemplate), valueText);
        }

        int questionDialog(QWidget *parent, const QString &actionText, const QString &itemText, const QString &templateText, const QString &valueText)
        {
            return QMessageBox::question(parent, QApplication::translate("Robomongo::DialogUtils", titleTemplate).arg(actionText).arg(itemText), templateText.arg(actionText).arg(itemText.toLower()).arg(valueText), QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);
        }
    }
}