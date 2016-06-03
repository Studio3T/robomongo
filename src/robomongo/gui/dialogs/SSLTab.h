#pragma once

#include <QWidget>

#include "robomongo/core/settings/ConnectionSettings.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QComboBox;
class QFrame;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;

    class SSLTab : public QWidget
    {
        Q_OBJECT

    public:
        SSLTab(ConnectionSettings *settings);

    private:

    };
}