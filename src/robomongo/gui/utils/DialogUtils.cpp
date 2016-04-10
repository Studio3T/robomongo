#include "robomongo/gui/utils/DialogUtils.h"

namespace Robomongo
{
    namespace utils
    {
        namespace
        {
            const QString titleTemaple = QString("%1 %2");
            const QString textTemaple = QString("%1 <b>%3</b> %2?");
        }

        int questionDialog(QWidget *parent, const QString &actionText, const QString &itemText, const QString& valueText)
        {
            return questionDialog(parent, actionText, itemText, textTemaple, valueText);
        }

        int questionDialog(QWidget *parent, const QString &actionText, const QString &itemText, const QString &templateText, const QString &valueText)
        {
            return QMessageBox::question(parent, titleTemaple.arg(actionText).arg(itemText), templateText.arg(actionText).arg(itemText.toLower()).arg(valueText), QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);
        }
    }
}