#pragma once

#include <QObject>
#include <mongo/bson/bsonobj.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class EventBus;

    /**
     * @brief Represents MongoDB database.
     */
    class MongoDatabase : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief MongoDatabase
         * @param server: pointer to parent MongoServer
         */
        MongoDatabase(MongoServer *server, const std::string &name);
        ~MongoDatabase();

        /**
         * @brief Initiate listCollection asynchronous operation.
         */
        void loadCollections();

        /**
         * @brief Initiate loadUsers asynchronous operation.
         */
        void loadUsers();

        void loadFunctions();

        void createCollection(const std::string &collection);
        void dropCollection(const std::string &collection);
        void renameCollection(const std::string &collection, const std::string &newCollection);
        void duplicateCollection(const std::string &collection, const std::string &newCollection);
        void copyCollection(MongoServer *server, const std::string &sourceDatabase, const std::string &collection);

        void createUser(const MongoUser &user, bool overwrite);
        void dropUser(const mongo::OID &id);

        void createFunction(const MongoFunction &fun);
        void updateFunction(const std::string &name, const MongoFunction &fun);
        void dropFunction(const std::string &name);

        const std::string &name() const { return _name; }

        /**
         * @brief Checks that this is a system database.
         * @return true if system, false otherwise.
         */
        bool isSystem() const { return _system; }

        MongoServer *server() const { return _server; }

    protected Q_SLOTS:
        void handle(LoadCollectionNamesResponse *event);
        void handle(LoadUsersResponse *event);
        void handle(LoadFunctionsResponse *event);
        void handle(CreateUserResponse *event);

    private:
        void clearCollections();
        void addCollection(MongoCollection *collection);

    private:
        MongoServer *_server;
        std::vector<MongoCollection *> _collections;
        const std::string _name;
        const bool _system;
        EventBus *_bus;
    };

    class MongoDatabaseCollectionListLoadedEvent : public Event
    {
        R_EVENT

        MongoDatabaseCollectionListLoadedEvent(QObject *sender, const std::vector<MongoCollection *> &list) :
            Event(sender),
            collections(list) { }

        std::vector<MongoCollection *> collections;
    };

    class MongoDatabaseUsersLoadedEvent : public Event
    {
        R_EVENT

        MongoDatabaseUsersLoadedEvent(QObject *sender, MongoDatabase *database, const std::vector<MongoUser> &list) :
            Event(sender),
            _users(list),
            _database(database) {}

        std::vector<MongoUser> users() const { return _users; }
        MongoDatabase *database() const { return _database; }

    private:
        std::vector<MongoUser> _users;
        MongoDatabase *_database;
    };

    class MongoDatabaseFunctionsLoadedEvent : public Event
    {
        R_EVENT

        MongoDatabaseFunctionsLoadedEvent(QObject *sender, MongoDatabase *database, const std::vector<MongoFunction> &list) :
            Event(sender),
            _functions(list),
            _database(database) {}

        std::vector<MongoFunction> functions() const { return _functions; }
        MongoDatabase *database() const { return _database; }

    private:
        std::vector<MongoFunction> _functions;
        MongoDatabase *_database;
    };

    class MongoDatabaseUsersLoadingEvent : public Event
    {
        R_EVENT
        MongoDatabaseUsersLoadingEvent(QObject *sender) : Event(sender) {}
    };

    class MongoDatabaseFunctionsLoadingEvent : public Event
    {
        R_EVENT
        MongoDatabaseFunctionsLoadingEvent(QObject *sender) : Event(sender) {}
    };

    class MongoDatabaseCollectionsLoadingEvent : public Event
    {
        R_EVENT
        MongoDatabaseCollectionsLoadingEvent(QObject *sender) : Event(sender) {}
    };
}
