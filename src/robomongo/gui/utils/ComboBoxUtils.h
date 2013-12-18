#pragma once

#include <QComboBox>

namespace Robomongo
{
    namespace utils
    {
        /**
         * @brief This function behaves identically to Qt5 QComboBox::setCurrentText().
         * We are using this function in order to support Qt4
         */
        void setCurrentText(QComboBox *comboBox, const QString &text);
    }
}
