#pragma once

#include <QWidget>

#include "robomongo/core/settings/ConnectionSettings.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QRadioButton;
class QComboBox;
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

        /**
        * @return true if use SSL checkbox is checked, false otherwise
        */
        bool sslEnabled() const;

        void clearTab();
        void setSslOptions(
            int index,
            bool allowInvalidHostnames,
            std::string_view caFile,
            std::string_view certPemFile,
            std::string_view certPemFilePwd 
         );

    private Q_SLOTS :
        
        /**
        * @brief Disable/enable widgets according to SSL check box state
        */ 
        void useSslCheckBoxStateChange(int checked);
        
        /**
        * @brief Disable/enable widgets according to state of authentication method combo box
        */
        void on_authModeComboBox_change(int index);

        /**
        * @brief Disable/enable widgets according to state of use PEM file combo box
        */
        void on_usePemFileCheckBox_toggle(bool checked);

        /**
        * @brief Disable/enable widgets according to state of advanced options combo box
        */
        void on_useAdvancedOptionsCheckBox_toggle(bool checked);
        
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
        * @brief Enable/disable/clean PEM passphrase widgets section
        */
        void on_askPemPassCheckBox_toggle(bool checked);

    private:

        /**
        * @brief Do validation according to user input in UI
        * @return true on success, false otherwise
        */
        bool validate();

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
        * @param initialPath Previously selected file path
        * @return Selected file absolute path and file name
        */
        QString openFileBrowseDialog(const QString& initialPath);

        /**
        * @brief Main checkbox to disable/enable all other SSL tab widgets
        */
        QCheckBox *_useSslCheckBox;
        
        /**
        * @brief Auth. Method widgets
        */
        QLabel *_authMethodLabel;
        QComboBox *_authMethodComboBox;
        QLabel *_selfSignedInfoStr;
        QLabel *_caFileLabel;
        QLineEdit *_caFilePathLineEdit;
        QPushButton *_caFileBrowseButton;

        /**
        * @brief PEM file widgets
        */
        QCheckBox* _usePemFileCheckBox;
        QLabel* _pemFileInfoStr;
        QLabel* _pemFileLabel;
        QLineEdit *_pemFilePathLineEdit;
        QPushButton *_pemFileBrowseButton;
        QLabel* _pemPassLabel;
        QLineEdit* _pemPassLineEdit;
        QPushButton* _pemPassShowButton;
        QCheckBox* _askPemPassCheckBox;

        /**
        * @brief Advanced options widgets
        */
        QCheckBox* _useAdvancedOptionsCheckBox;
        QLabel *_crlFileLabel;
        QLineEdit *_crlFilePathLineEdit;
        QPushButton *_crlFileBrowseButton;
        QLabel *_allowInvalidHostnamesLabel;
        QComboBox *_allowInvalidHostnamesComboBox;

        /**
        * @brief Pointer to active connection's settings
        */
        ConnectionSettings *const _connSettings;
    };
} /* end of Robomongo namespace */
