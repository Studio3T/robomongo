#include "robomongo/gui/utils/ComboBoxUtils.h"

namespace Robomongo
{
    namespace utils
    {
        /**
         * @brief This function behaves identically to Qt5 QComboBox::setCurrentText().
         * We are using this function in order to support Qt4
         */
        void setCurrentText(QComboBox *comboBox, const QString &text)
        {
            if (comboBox->isEditable()) {
                comboBox->setEditText(text);
            } else {
                const int i = comboBox->findText(text);
                if (i > -1)
                    comboBox->setCurrentIndex(i);
            }
        }
    }
}
