#pragma once

#include <QWidget>

#include "robomongo/core/settings/ConnectionSettings.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QRadioButton;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;

    class SSLTab : public QWidget
    {
        Q_OBJECT

    public:
        /**
        * @brief Construct SSL tab of ConnectionDialog
        * @param settings: Pointer to active connection's ConnectionsSettings member
        */
        SSLTab(ConnectionSettings *settings);
        
        /**
        * @brief  Save dialog input into connection's ssl settings
        * @return true on success, false otherwise
        */
        bool accept();

    private Q_SLOTS :
        
        /**
        * @brief Disable/enable widgets according to SSL checkbox state
        */ 
        void useSslCheckBoxStateChange(int checked);
        
        /**
        * @brief Disable/enable widgets according to state of self-signed radio button
        */
        void on_acceptSelfSignedButton_toggle(bool checked);
        
        /**
        * @brief File browsers for SSL related cert/key files
        */
        void on_caFileBrowseButton_clicked();
        void on_pemKeyFileBrowseButton_clicked();
        void on_crlFileBrowseButton_clicked();
        
        /**
        * @brief Show/hide client cert's passphrase on Show/Hide button pressed
        */
        void togglePassphraseShowMode();
        
        /**
        * @brief Enable/disable client cert's passphrase widget section
        */
        void on_useClientCertPassCheckBox_toggle(bool checked);

    private:

        /**
        * @brief Do validation according to user input in UI
        * @return true on success, false otherwise
        */
        bool validate();

        /**
        * @brief Enable/disable Use CA file widget section
        */
        void setDisableCAfileWidgets(bool disabled);

        /**
        * @brief  Check existence of files: CA cert, Client Cert and CRL file
        * @return true if all files exist, false any of them does not exist
        * @return QString Lable of file which does not exist, empty string if all files exist
        */
        std::pair<bool, QString> checkExistenseOfFiles() const;

        /**
        * @brief Save dialog input into connection's ssl settings
        */
        void saveSslSettings() const;

        /**
        * @brief 
        */
        QString openFileBrowseDialog(const QString& initialPath);

        /**
        * @brief Main checkbox to disable/enable all other SSL tab widgets
        */
        QCheckBox *_useSslCheckBox;
        
        /**
        * @brief CA file widgets
        */
        QRadioButton *_acceptSelfSignedButton;
        QRadioButton *_useRootCaFileButton;
        QLabel *_caFileLabel;
        QLineEdit *_caFilePathLineEdit;
        QPushButton *_caFileBrowseButton;

        /**
        * @brief Client Certificate file widgets
        */
        QCheckBox *_useClientCertCheckBox;
        QLabel *_clientCertLabel;
        QLineEdit *_clientCertPathLineEdit;
        QPushButton *_clientCertFileBrowseButton;
        QLabel *_clientCertPassLabel;
        QLineEdit *_clientCertPassLineEdit;
        QPushButton *_clientCertPassShowButton;
        QCheckBox *_useClientCertPassCheckBox;

        /**
        * @brief CRL file widgets
        */
        QLabel *_crlFileLabel;
        QLineEdit *_crlFilePathLineEdit;
        QPushButton *_crlFileBrowseButton;
        
        /**
        * @brief Allow connections with non-matching hostnames
        */
        QCheckBox *_allowInvalidHostnamesCheckBox;

        /**
        * @brief Pointer to active connection's ConnectionsSettings member
        */
        ConnectionSettings *const _settings;
    };
} /* end of Robomongo namespace */
