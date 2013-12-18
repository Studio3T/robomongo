#include "robomongo/gui/utils/ComboBoxUtils.h"

namespace Robomongo
{
    namespace utils
    {
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
