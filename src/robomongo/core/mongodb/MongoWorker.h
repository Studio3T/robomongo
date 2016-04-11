#pragma once

#include <QObject>
#include <QMutex>
#include <unordered_set>

#include "robomongo/core/events/MongoEvents.h"

QT_BEGIN_NAMESPACE
class QThread;
class QTimer;
QT_END_NAMESPACE

namespace Robomongo
{
    class MongoClient;
    class ScriptEngine;
    class ConnectionSettings;

    class MongoWorker : public QObject
    {
        Q_OBJECT

    public:
        typedef std::vector<std::string> DatabasesContainerType;
        explicit MongoWorker(ConnectionSettings *connection, bool isLoadMongoRcJs, int batchSize,
                             int mongoTimeoutSec, int shellTimeoutSec, QObject *parent = NULL);
        ~MongoWorker();
        enum { pingTimeMs = 60 * 1000 };
        void interrupt();
        void stopAndDelete();
        
    protected Q_SLOTS: // handlers:
        void init();
        /**
         * @brief Every minute we are issuing { ping : 1 } command to every used connection
         * in order to avoid dropped connections.
         */
        void keepAlive();

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
        void handle(StopScriptRequest *event);

        void handle(AutocompleteRequest *event);
        void handle(CreateDatabaseRequest *event);
        void handle(DropDatabaseRequest *event);

        void handle(CreateCollectionRequest *event);
        void handle(DropCollectionRequest *event);
        void handle(RenameCollectionRequest *event);
        void handle(DuplicateCollectionRequest *event);
        void handle(CopyCollectionToDiffServerRequest *event);

        void handle(CreateUserRequest *event);
        void handle(DropUserRequest *event);

        void handle(CreateFunctionRequest *event);
        void handle(DropFunctionRequest *event);

    protected:
        virtual void timerEvent(QTimerEvent *);

    private:
        /**
         * @brief Send event to this MongoWorker
         */
        void send(Event *event);
        DatabasesContainerType getDatabaseNamesSafe();
        std::string getAuthBase() const;

        mongo::DBClientBase *_dbclient;
        mongo::DBClientBase *getConnection(bool mayReturnNull = false);
        MongoClient *getClient();

        /**
         * @brief Send reply event to object 'obj'
         */
        void reply(QObject *receiver, Event *event);
        QThread *_thread;
        QMutex _firstConnectionMutex;

        ScriptEngine *_scriptEngine;

        bool _isAdmin;
        const bool _isLoadMongoRcJs;
        const int _batchSize;
        int _timerId;
        int _mongoTimeoutSec;
        int _shellTimeoutSec;
        QAtomicInteger<int> _isQuiting;

        ConnectionSettings *_connection;

        // Collection of created databases.
        // Starting from 3.0, MongoDB drops empty databases.
        // It means, we did not find a way to create "empty" database.
        // We save all created databases in this collection and merge with
        // list of real databases returned from MongoDB server.
        std::unordered_set<std::string> _createdDbs;
    };

}
