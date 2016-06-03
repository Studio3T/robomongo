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
class QRadioButton;
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
        QCheckBox *_useSslCheckBox;

        QRadioButton *_acceptSelfSignedButton;
        QRadioButton *_useRootCaFileButton;
        QLabel *_caFileLabel;
        QLineEdit *_caFilePathLineEdit;
        QPushButton *_caFileBrowseButton;

        QCheckBox *_useClientCertCheckBox;
        QLabel *_clientCertLabel;
        QLineEdit *_clientCertPathLineEdit;
        QPushButton *_clientCertFileBrowseButton;
        QLabel *_clientCertPassLabel;
        QLineEdit *_clientCertPassLineEdit;
        QPushButton *_clientCertPassShowButton;
        QCheckBox *_useClientCertPassCheckBox;

        ConnectionSettings *const _settings;
    };
}
