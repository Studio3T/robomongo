#pragma once
#include <QObject>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class MongoWorker;
    class MongoDatabase;
    class EventBus;
    class App;

    // Messages
    struct EstablishConnectionResponse;
    struct RefreshReplicaSetFolderResponse;
    class LoadDatabaseNamesResponse;
    class InsertDocumentResponse;
    struct CreateDatabaseResponse;
    struct DropDatabaseResponse;

    /**
     * @brief MongoServer represents active connection to MongoDB server.
     * MongoServer is an Aggregate Root, that manages three internal entities:
     * MongoDatabase, MongoCollection and MongoWorker.
     */
    class MongoServer : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief MongoServer
         * @param connectionRecord: MongoServer will own this ConnectionSettings.
         * @param visible
         * @param defaultDatabase
         */
        MongoServer(int handle, ConnectionSettings *connectionRecord, ConnectionType connectionType);
        ~MongoServer();

        void runWorkerThread();

        /**
         * @brief Try to connect to MongoDB single server or replica set.
         * @throws MongoException, if fails
         */
        void tryConnect();

        /**
        * @brief Try to re-connect to MongoDB server in order to refresh connection view.
        *        Never shown in Explorer and can be used to refresh (via reconnecting) current connection view.
        *        (i.e. db version, storage engine etc...)
        * @throws MongoException, if fails
        */
        void tryRefresh();

        /**
        * @brief Try to re-connect to MongoDB replica set in order to refresh connection view.
        *        It is used to refresh (via reconnecting) current connection view.
        *        (i.e. db version, storage engine, current replica set primary, status of replica set etc...)
        *        It will update Explorer widgets depending on replica set status (online if primary reachable and
        *        offline otherwise)
        */
        void tryRefreshReplicaSetConnection();

        /**
        * @brief Try to re-connect to MongoDB replica set in order to refresh connection view.
        *        It is used to refresh (via reconnecting) current connection view.
        *        (i.e. db version, storage engine, current replica set primary, status of replica set etc...)
        *        It will update only 'Replica Set' folder widgets depending on replica set status 
        *       (online if primary reachable and offline otherwise)
        */
        void tryRefreshReplicaSetFolder(bool expanded, bool showLoading = true);

        bool isConnected() const;

        void addDatabase(MongoDatabase *database);
        void createDatabase(const std::string &dbName);
        void dropDatabase(const std::string &dbName);
        QStringList getDatabasesNames() const;
        QList<MongoDatabase*> const& databases() const { return _databases; };
        MongoDatabase *findDatabaseByName(const std::string &dbName) const;

        void insertDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns);
        void insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns);
        void saveDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns);
        void saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns);
        void removeDocuments(mongo::Query query, const MongoNamespace &ns, RemoveDocumentCount removeCount, 
                             int index = 0);
        float version() const{ return _version; }
        const std::string& getStorageEngineType() const { return _storageEngineType; }

        /**
         * @brief Returns associated connection record
         */
        ConnectionSettings *connectionRecord() const;

        /**
         * @brief Loads databases of this server asynchronously.
         */
        void loadDatabases();
        MongoWorker *const worker() const { return _worker; }

        ReplicaSet* replicaSetInfo() const { return _replicaSetInfo.get(); }

        void handle(ReplicaSetRefreshed *event);

        void changeWorkerShellTimeout(int newTimeout);

    protected Q_SLOTS:
        void handle(EstablishConnectionResponse *event);
        void handle(RefreshReplicaSetFolderResponse *event);
        void handle(LoadDatabaseNamesResponse *event);
        void handle(InsertDocumentResponse *event);
        void handle(RemoveDocumentResponse *event);
        void handle(CreateDatabaseResponse *event);
        void handle(DropDatabaseResponse *event);

    private:                 
        void clearDatabases();
        void handleReplicaSetRefreshEvents(bool isError, EventError eventError, ReplicaSet const& replicaSet,
                                           bool expanded);
        void updateReplicaSetSettings(EstablishConnectionResponse* event);
        void handleConnectionFailure(EstablishConnectionResponse* event);

        MongoWorker *_worker;
        std::unique_ptr<ConnectionSettings> _connSettings;
        EventBus *_bus;
        App *_app;

        float _version;
        std::string _storageEngineType;
        ConnectionType _connectionType;
        bool _isConnected;
        int _handle;

        QList<MongoDatabase *> _databases;
        std::unique_ptr<ReplicaSet> _replicaSetInfo;
    };

    class MongoServerLoadingDatabasesEvent : public Event
    {
        R_EVENT
        MongoServerLoadingDatabasesEvent(QObject *sender) : Event(sender) {}
    };
}
