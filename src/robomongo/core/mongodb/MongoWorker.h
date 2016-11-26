#pragma once

#include <QObject>
#include <QMutex>
#include <unordered_set>

#include <mongo/client/dbclient_rs.h> // todo: move

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
        using upDBClientReplicaSet = std::unique_ptr<mongo::DBClientReplicaSet>;
        using upDBClientConnection = std::unique_ptr<mongo::DBClientConnection>;

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
        bool handle(EstablishConnectionRequest *event);

        /**
        * @brief todo
        */
        void handle(RefreshReplicaSetRequest *event);

        /**
        * @brief todo
        */
        void handle(RefreshReplicaSetFolderRequest *event);

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

        mongo::DBClientBase *getConnection(bool mayReturnNull = false);
        MongoClient *getClient();

        /**
        *@brief Update global mongo SSL settings (mongo::sslGlobalParams) according to active connection 
        *       request's SSL settings.
        */
        void updateGlobalSSLparams() const;

        /**
        *@brief Reset global mongo SSL settings (mongo::sslGlobalParams) into default zero state
        */
        void resetGlobalSSLparams() const;

        /**
        *@brief Update Replica Set related parameters/settings.
        *       Warning: Using refresh false might return not-updated values.
        */
        ReplicaSet getReplicaSetInfo(bool refresh = true) const;   // todo: throws ??

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
        int _dbAutocompleteCacheTimerId;
        int _mongoTimeoutSec;
        int _shellTimeoutSec;
        QAtomicInteger<int> _isQuiting;

        std::unique_ptr<mongo::DBClientConnection> _dbclient;
        std::unique_ptr<mongo::DBClientReplicaSet> _dbclientRepSet;

        ConnectionSettings *_connSettings;

        // Collection of created databases.
        // Starting from 3.0, MongoDB drops empty databases.
        // It means, we did not find a way to create "empty" database.
        // We save all created databases in this collection and merge with
        // list of real databases returned from MongoDB server.
        std::unordered_set<std::string> _createdDbs;
    };

}
