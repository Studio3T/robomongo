#pragma once

#include <QObject>
#include <mongo/bson/bsonobj.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class EventBus;
    class MongoServer;
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
        virtual void customEvent(QEvent *);

    Q_SIGNALS:
        void startedCollectionListLoad();
        void collectionListLoaded(std::vector<MongoCollection *>);
        void startedUsersLoad();
        void userListLoaded(std::vector<MongoUser>);
        void startedFunctionsLoad();
        void functionsListLoaded(std::vector<MongoFunction>);
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
}
