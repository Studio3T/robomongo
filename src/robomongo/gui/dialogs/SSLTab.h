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
        //
        SSLTab(ConnectionSettings *settings);
        //
        bool accept();

    private Q_SLOTS :
        //
        void useSslCheckBoxStateChange(int checked);
        //
        void on_acceptSelfSignedButton_toggle(bool checked);
        //
        void on_caFileBrowseButton_clicked();
        //
        void on_pemKeyFileBrowseButton_clicked();
        //
        void on_crlFileBrowseButton_clicked();
        //
        void togglePassphraseShowMode();

    private:

        //
        void setDisabledCAfileWidgets(bool disabled);


        // Widgets
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

        QCheckBox *_allowInvalidHostnamesCheckBox;
        QLabel *_crlFileLabel;
        QLineEdit *_crlFilePathLineEdit;
        QPushButton *_crlFileBrowseButton;

        ConnectionSettings *const _settings;
    };
}
