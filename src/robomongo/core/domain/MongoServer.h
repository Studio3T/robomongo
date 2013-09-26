#pragma once
#include <QObject>

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
        typedef QList<MongoDatabase *> DatabasesContainerType;
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
        MongoDatabase *findDatabaseByName(const std::string &dbName) const;

        void insertDocuments(const std::vector<mongo::BSONObj> &objCont, const std::string &db, const std::string &collection);
        void insertDocument(const mongo::BSONObj &obj, const std::string &db, const std::string &collection);
        void saveDocuments(const std::vector<mongo::BSONObj> &objCont, const std::string &db, const std::string &collection);
        void saveDocument(const mongo::BSONObj &obj, const std::string &db, const std::string &collection);
        void removeDocuments(mongo::Query query, const std::string &db, const std::string &collection, bool justOne = true);
        float version() const{ return _version; }


        /**
         * @brief Returns associated connection record
         */
        ConnectionSettings *connectionRecord() const;

        /**
         * @brief Loads databases of this server asynchronously.
         */
        void loadDatabases();
        bool visible() const { return _visible; }
        MongoWorker *const client() const { return _client; }

    protected Q_SLOTS:
        void handle(EstablishConnectionResponse *event);
        void handle(LoadDatabaseNamesResponse *event);
        void handle(InsertDocumentResponse *event);

    private:
        void clearDatabases();
        void addDatabase(MongoDatabase *database);

        MongoWorker *const _client;

        float _version;
        bool _visible;

        DatabasesContainerType _databases;
    };

    class MongoServerLoadingDatabasesEvent : public Event
    {
        R_EVENT
        MongoServerLoadingDatabasesEvent(QObject *sender) : Event(sender) {}
    };
}
