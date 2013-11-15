#pragma once

#include <QObject>

#include "robomongo/core/Core.h"
#include "robomongo/core/events/MongoEventsInfo.hpp"

namespace Robomongo
{
    /**
     * @brief Represents MongoDB database.
     */
    class MongoDatabase : public QObject
    {
        Q_OBJECT

    public:
        typedef std::vector<MongoCollection *> CollectionsContainerType;
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
        CollectionsContainerType collections() const { return _collections; }

    Q_SIGNALS:
        void startedCollectionListLoad(const EventsInfo::LoadCollectionRequestInfo &inf);
        void finishedCollectionListLoad(const EventsInfo::LoadCollectionResponceInfo &inf);

        void startedUserListLoad(const EventsInfo::LoadUserRequestInfo &inf);
        void finishedUserListLoad(const EventsInfo::LoadUserResponceInfo &inf);

        void startedFunctionListLoad(const EventsInfo::LoadFunctionRequestInfo &inf);
        void finishedFunctionListLoad(const EventsInfo::LoadFunctionResponceInfo &inf);

    private:
        void clearCollections();

    private:
        MongoServer *_server;
        CollectionsContainerType _collections;
        const std::string _name;
        const bool _system;
    };
}
