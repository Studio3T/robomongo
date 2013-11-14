#pragma once
#include <QObject>

#include "robomongo/core/events/MongoEventsInfo.hpp"

namespace Robomongo
{
    class MongoDatabase;

    class MongoCollection : public QObject
    {
        Q_OBJECT
    public:
        typedef QObject BaseClass;
        MongoCollection(MongoDatabase *database, const MongoCollectionInfo &info);

        bool isSystem() const { return _system; }

        std::string name() const { return _ns.collectionName(); }
        const MongoCollectionInfo info() const { return _info; }
        std::string fullName() const { return _ns.toString(); }
        MongoDatabase *database() const { return _database; }

        std::string sizeString() const;
        QString storageSizeString() const;
        void loadIndexes();
        void createIndex(const EnsureIndex &oldInfo, const EnsureIndex &newInfo);
        void dropIndex(const std::string &indexName);

    Q_SIGNALS:
        void startedIndexListLoad(const EventsInfo::LoadCollectionIndexesInfo &inf);
        void finishedIndexListLoad(const EventsInfo::LoadCollectionIndexesInfo &inf);

    protected:
       virtual void customEvent(QEvent *);

    private:

        MongoDatabase *_database;
        bool _system;
        MongoCollectionInfo _info;
        MongoNamespace _ns;
    };
}
