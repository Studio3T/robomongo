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
    * @brief This class is not finished, it is still under development.
    *        It was planned to be used as very first dialog right after 'create' new
    *        connection is clicked on connections dialog in order to provide user 
    *        some create more friendly create connection options with pictures 
    *        e.g. create connection from URI string, create replica set or create single server.
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

        // todo:
        // mongo::MongoURI getMongoUri() const { return _mongoUri; }
        bool fromURI() const { return _fromURI; }

        std::unique_ptr<mongo::StatusWith<mongo::MongoURI>> getMongoUriWithStatus() 
        { 
            return std::move(_mongoUriWithStatus); 
        }

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

        std::unique_ptr<mongo::StatusWith<mongo::MongoURI>> _mongoUriWithStatus;
        bool _fromURI;
    };
}
