#pragma once

#include <QDialog>
#include <QTreeWidget>

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

    protected:
        /**
         * @brief Close event handler
         */
        void closeEvent(QCloseEvent *);

    private slots:
        /**
         * @brief Test current connection
         */
        void testConnection();

        void tabWidget_currentChanged(int index);

    private:
        ConnectionAuthTab *_authTab;
        ConnectionBasicTab *_basicTab;
        ConnectionAdvancedTab *_advancedTab;
        QTabWidget *_tabWidget;

        /**
         * @brief Edited connection
         */
        ConnectionSettings *_connection;

        /**
         * @brief Check that it is okay to close this window
         *        (there is no modification of data, that we possibly can loose)
         */
        bool canBeClosed();
    };
}
