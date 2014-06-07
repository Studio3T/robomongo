#pragma once

#include <QApplication>
#include <QMessageBox>

namespace Robomongo
{
    namespace utils
    {
        int questionDialog(QWidget *parent, const QString &actionText, const QString &itemText, const QString &valueText);

        int questionDialog(QWidget *parent, const QString &actionText, const QString &itemText, const QString &templateText, const QString &valueText);
    }
}

