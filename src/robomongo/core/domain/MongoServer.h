#pragma once
#include <QObject>
#include <QList>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class MongoWorker;
    class MongoDatabase;
    class EstablishConnectionResponse;
    class LoadDatabaseNamesResponse;
    class InsertDocumentResponse;

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
        typedef QList<MongoDatabase *> databasesContainerType;
        MongoServer(ConnectionSettings *connectionRecord, bool visible);
        ~MongoServer();

        /**
         * @brief Try to connect to MongoDB server.
         * @throws MongoException, if fails
         */
        void tryConnect();

        void createDatabase(const std::string &dbName);
        void dropDatabase(const std::string &dbName);
        QStringList getDatabasesNames() const;

        void insertDocuments(const std::vector<mongo::BSONObj> &objCont, const std::string &db, const std::string &collection);
        void insertDocument(const mongo::BSONObj &obj, const std::string &db, const std::string &collection);
        void saveDocuments(const std::vector<mongo::BSONObj> &objCont, const std::string &db, const std::string &collection);
        void saveDocument(const mongo::BSONObj &obj, const std::string &db, const std::string &collection);
        void removeDocuments(mongo::Query query, const std::string &db, const std::string &collection, bool justOne = true);

        /**
         * @brief Returns last error message
         */
        std::string lastErrorMessage() { return _lastErrorMessage; }

        /**
         * @brief Returns associated connection record
         */
        ConnectionSettings *connectionRecord() const { return _connectionRecord; }

        /**
         * @brief Loads databases of this server asynchronously.
         */
        void loadDatabases();

        MongoWorker *client() const { return _client.data(); }

    protected Q_SLOTS:
        void handle(EstablishConnectionResponse *event);
        void handle(LoadDatabaseNamesResponse *event);

    private:
        void clearDatabases();
        void addDatabase(MongoDatabase *database);

        QScopedPointer<MongoWorker> _client;
        DBClientConnectionScopedPtr _connection;

        /**
         * @brief Associated connection record
         */
        ConnectionSettings *_connectionRecord;

        std::string _host;
        std::string _port;
        std::string _address;
        std::string _lastErrorMessage;
        bool _visible;

        databasesContainerType _databases;

        EventBus *_bus;
    };

    class MongoServerLoadingDatabasesEvent : public Event
    {
        R_EVENT
        MongoServerLoadingDatabasesEvent(QObject *sender) : Event(sender) {}
    };
}
