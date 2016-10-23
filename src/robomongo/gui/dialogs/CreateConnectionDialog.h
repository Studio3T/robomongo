#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

#include "robomongo/core/Core.h"

#include <mongo/client/mongo_uri.h>

namespace Robomongo
{
    class ConnectionsDialog;
    class ConnectionSettings;

    /**
    * @brief This Dialog allows to edit single connection
    */
    class CreateConnectionDialog : public QDialog
    {
        Q_OBJECT

    public:
        using MongoURIwithStatus = mongo::StatusWith<mongo::MongoURI>;

        /**
        * @brief Constructs dialog with specified connection
        */
        CreateConnectionDialog(ConnectionsDialog *parent);
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

        void on_createConnLinkActivated();

    private:
        bool validateAndApply();

        // todo: may return nullptr
        /* example usage:
                auto uriWithStatus = getUriWithStatus();
                 if (!uriWithStatus || !uriWithStatus->isOK()) {
                    return;
                }
                auto uri = uriWithStatus->getValue();
        */
        std::unique_ptr<MongoURIwithStatus> getUriWithStatus();

        QLineEdit* _uriLineEdit;

        /**
        * @brief Edited connection
        */
        ConnectionSettings* _connection;
        
        ConnectionsDialog* _connectionsDialog;
    };
}
