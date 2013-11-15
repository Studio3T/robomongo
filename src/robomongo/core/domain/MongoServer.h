#pragma once
#include <QObject>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/events/MongoEventsInfo.hpp"

namespace Robomongo
{
    class MongoWorker;
    class MongoDatabase;
    class EstablishConnectionResponse;

    /**
     * @brief MongoServer represents active connection to MongoDB server.
     * MongoServer is an Aggregate Root, that manages three internal entities:
     * MongoDatabase, MongoCollection and MongoWorker.
     */
    class MongoServer : public QObject
    {
        Q_OBJECT
        friend class MongoWorker;
    public:
        /**
         * @brief MongoServer
         * @param connectionRecord: MongoServer will own this ConnectionSettings.
         * @param visible
         * @param defaultDatabase
         */
        typedef QObject BaseClass;
        typedef QList<MongoDatabase *> DatabasesContainerType;
        MongoServer(const IConnectionSettingsBase *connectionRecord, bool visible);
        ~MongoServer();

        /**
         * @brief Try to connect to MongoDB server.
         * @throws MongoException, if fails
         */
        void tryConnect();
        bool isConnected()const;

        void createDatabase(const std::string &dbName);
        void dropDatabase(const std::string &dbName);

        QStringList getDatabasesNames() const;
        MongoDatabase *findDatabaseByName(const std::string &dbName) const;

        void insertDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns);
        void insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns);
        void saveDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns);
        void saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns);
        void removeDocuments(mongo::Query query, const MongoNamespace &ns, bool justOne = true);
        void query(int resultIndex, const MongoQueryInfo &info);

        float version() const{ return _version; }

        /**
         * @brief Returns associated connection record
         */
        const IConnectionSettingsBase *connectionRecord() const;

        /**
         * @brief Loads databases of this server asynchronously.
         */
        void loadDatabases();
        bool visible() const { return _visible; }
        void postEventToDataBase(QEvent *event, int priority = Qt::NormalEventPriority) const;
        DatabasesContainerType databases() const { return _databases; }

    Q_SIGNALS:
        void startConnected(const EventsInfo::EstablishConnectionRequestInfo &inf);
        void finishConnected(const EventsInfo::EstablishConnectionResponceInfo &inf);

        void startLoadDatabases(const EventsInfo::LoadDatabaseNamesRequestInfo &inf);
        void finishLoadDatabases(const EventsInfo::LoadDatabaseNamesResponceInfo &inf);

        void documentListLoaded(const EventsInfo::ExecuteQueryResponceInfo &inf);

    protected:
        virtual void customEvent(QEvent *);

    private:
        void clearDatabases();
        void addDatabase(MongoDatabase *database);

        MongoWorker *const _client;

        float _version;
        bool _visible;
        bool _isConnected;

        DatabasesContainerType _databases;
    };
}
