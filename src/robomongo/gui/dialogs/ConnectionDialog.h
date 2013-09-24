#pragma once

#include <QDialog>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class ConnectionSettings;
    class ConnectionAuthTab;
    class ConnectionBasicTab;
    class ConnectionAdvancedTab;
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

        /**
         * @brief Accept() is called when user agree with entered data.
         */
        virtual void accept();
        void apply();

        ConnectionSettings *connection() const { return _connection; }

    private Q_SLOTS:
        /**
         * @brief Test current connection
         */
        void testConnection();

    private:
        ConnectionAuthTab *_authTab;
        ConnectionBasicTab *_basicTab;
        ConnectionAdvancedTab *_advancedTab;

        /**
         * @brief Edited connection
         */
        ConnectionSettings *_connection;
    };
}
