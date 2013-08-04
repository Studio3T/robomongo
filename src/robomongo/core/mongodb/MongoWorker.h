#pragma once

#include <QObject>
#include <QMutex>
QT_BEGIN_NAMESPACE
class QThread;
class QTimer;
QT_END_NAMESPACE

#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class MongoClient;
    class ScriptEngine;
    class ConnectionSettings;

    class MongoWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit MongoWorker(ConnectionSettings *connection, QObject *parent = NULL);

        ~MongoWorker();

        /**
         * @brief Send event to this MongoWorker
         */
        void send(Event *event);
    protected Q_SLOTS: // handlers:

        /**
         * @brief Every minute we are issuing { ping : 1 } command to every used connection
         * in order to avoid dropped connections.
         */
        void keepAlive();

        /**
         * @brief Initialize MongoWorker (should be the first request)
         */
        void handle(InitRequest *event);

        /**
         * @brief Initialize MongoWorker (should be the first request)
         */
        void handle(FinalizeRequest *event);

        /**
         * @brief Initiate connection to MongoDB
         */
        void handle(EstablishConnectionRequest *event);

        /**
         * @brief Load list of all database names
         */
        void handle(LoadDatabaseNamesRequest *event);

        /**
         * @brief Load list of all collection names
         */
        void handle(LoadCollectionNamesRequest *event);

        /**
         * @brief Load list of all users
         */
        void handle(LoadUsersRequest *event);

        /**
        * @brief Load indexes in collection
        */
        void handle(LoadCollectionIndexesRequest *event);

        /**
        * @brief Load indexes in collection
        */
        void handle(EnsureIndexRequest *event);

        /**
        * @brief delete index from collection
        */
        void handle(DropCollectionIndexRequest *event);

          /**
        * @brief Edit index
        */
        void handle(EditIndexRequest *event);

        /**
         * @brief Load list of all JS functions
         */
        void handle(LoadFunctionsRequest *event);

        /**
         * @brief Inserts document
         */
        void handle(InsertDocumentRequest *event);

        /**
         * @brief Remove documents
         */
        void handle(RemoveDocumentRequest *event);

        /**
         * @brief Load list of all collection names
         */
        void handle(ExecuteQueryRequest *event);

        /**
         * @brief Execute javascript
         */
        void handle(ExecuteScriptRequest *event);

        void handle(AutocompleteRequest *event);
        void handle(CreateDatabaseRequest *event);
        void handle(DropDatabaseRequest *event);

        void handle(CreateCollectionRequest *event);
        void handle(DropCollectionRequest *event);
        void handle(RenameCollectionRequest *event);
        void handle(DuplicateCollectionRequest *event);

        void handle(CreateUserRequest *event);
        void handle(DropUserRequest *event);

        void handle(CreateFunctionRequest *event);
        void handle(DropFunctionRequest *event);

    private:

        mongo::DBClientBase *_dbclient;
        mongo::DBClientBase *getConnection();
        MongoClient *getClient();

        /**
         * @brief Initialise MongoWorker
         */
        void init();

        /**
         * @brief Send reply event to object 'obj'
         */
        void reply(QObject *receiver, Event *event);

        const QString _address;
        QThread *_thread;
        QMutex _firstConnectionMutex;

        ScriptEngine *_scriptEngine;

        bool _isAdmin;
        QString _authDatabase;

        ConnectionSettings *_connection;

        /**
         * @brief Every minute we are issuing { ping : 1 } command to every used connection
         * in order to avoid dropped connections.
         */
        QTimer *_keepAliveTimer;
    };

}
