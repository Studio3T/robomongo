#pragma once

#include <QDialog>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class ConnectionSettings;
    class ConnectionAuthTab;
    class ConnectionBasicTab;
    class ConnectionAdvancedTab;
    class SshTunelTab;
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
        ConnectionDialog(ConnectionSettings *connection);

        ConnectionSettings *const connection() const { return _connection; }

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
        void apply();
        ConnectionAuthTab *_authTab;
        ConnectionBasicTab *_basicTab;
        ConnectionAdvancedTab *_advancedTab;
#ifdef OPENSSH_SUPPORT_ENABLED
        SshTunelTab *_sshTab;
#endif
        /**
         * @brief Edited connection
         */
        ConnectionSettings *const _connection;
    };
}
