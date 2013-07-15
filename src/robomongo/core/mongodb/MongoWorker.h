#pragma once

#include <QObject>
#include <QMutex>
#include <QEvent>
#include <QTimer>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/engine/ScriptEngine.h"

namespace Robomongo
{
    class Helper;
    class EventBus;
    class MongoWorkerThread;
    class MongoClient;

    class MongoWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit MongoWorker(EventBus *bus, ConnectionSettings *connection, QObject *parent = NULL);

        ~MongoWorker();

        /**
         * @brief Send event to this MongoWorker
         */
        void send(Event *event);
        ScriptEngine *engine() const { return _scriptEngine; }
    protected slots: // handlers:

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
        void handle(DeleteCollectionIndexRequest *event);

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

        QString _address;
        MongoWorkerThread *_thread;
        QMutex _firstConnectionMutex;

        ScriptEngine *_scriptEngine;
        Helper *_helper;

        bool _isAdmin;
        QString _authDatabase;

        ConnectionSettings *_connection;
        EventBus *_bus;

        /**
         * @brief Every minute we are issuing { ping : 1 } command to every used connection
         * in order to avoid dropped connections.
         */
        QTimer *_keepAliveTimer;
    };

    class Helper : public QObject
    {
        Q_OBJECT
    public:

        Helper() : QObject() {}
        QString text() const { return _text; }
        void clear() { _text = ""; }

    public slots:
        void print(const QString &message)
        {
            _text.append(message);
        }

    private:
        QString _text;
    };

}
