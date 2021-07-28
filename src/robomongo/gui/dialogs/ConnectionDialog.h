#pragma once

#include <QDialog>

#include "robomongo/core/Core.h"
#include "robomongo/gui/utils/GuiConstants.h"

namespace Robomongo
{
    class ConnectionSettings;
    class ConnectionAuthTab;
    class ConnectionBasicTab;
    class ConnectionAdvancedTab;
    class SSLTab;
    class SshTunnelTab;

    /**
     * @brief This Dialog allows to edit single connection
     */
    class ConnectionDialog : public QDialog
    {
        Q_OBJECT

    public:
        /**
         * @brief Constructs dialog with specified connection
         */
        ConnectionDialog(ConnectionSettings *connection, QWidget *parent = nullptr);
        
        ConnectionSettings *const connection() const { return _connection; }        
        void setAuthTab(
            QString const& db, 
            QString const& username, 
            QString const& pwd, 
            AuthMechanism authMech
        );
        void setDefaultDb(QString const& defaultDb);
        void toggleSshSupport(bool isReplicaSet);
        void clearConnAuthTab();
        
        void clearSslTab();
        void setSslTab(
            int index,
            bool allowInvalidHostnames,
            std::string_view caFile,
            std::string_view certPemFile,
            std::string_view certPemFilePwd
        );

    public Q_SLOTS:
        /**
         * @brief Accept() is called when user agree with entered data.
         */
        virtual void accept();       
        
    private Q_SLOTS:
        /**
         * @brief Test current connection
         */
        void testConnection();

    private:
        void reject();
        void closeEvent(QCloseEvent *event);
        void restoreWindowSettings();
        void saveWindowSettings() const;
        bool validateAndApply();
        
        ConnectionAuthTab *_authTab = nullptr;
        ConnectionBasicTab *_basicTab = nullptr;
        ConnectionAdvancedTab *_advancedTab = nullptr;
        SshTunnelTab *_sshTab = nullptr;
        SSLTab *_sslTab = nullptr;

#ifdef SSH_SUPPORT_ENABLED
        SshTunnelTab *_sshTab;
#endif
        /**
         * @brief Edited connection
         */
        ConnectionSettings *const _connection;
    };
}
