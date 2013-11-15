#include "robomongo/core/domain/MongoCollection.h"

#include "robomongo/core/domain/MongoUtils.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/events/MongoEvents.hpp"

namespace Robomongo
{
    MongoCollection::MongoCollection(MongoDatabase *database, const MongoCollectionInfo &info) :
        _ns(info.ns()),
        _database(database),
        _info(info),
        _system(false)
    {
        // System databases starts from system.*
        std::string collectionName = _ns.collectionName();
        std::string prefix = "system.";

        // Checking whether `collectionName` starts from `system`
        if (collectionName.compare(0, prefix.length(), prefix) == 0)
            _system = true;
    }

    std::string MongoCollection::sizeString() const
    {
        return MongoUtils::buildNiceSizeString(_info.sizeBytes()).toStdString();
    }

    QString MongoCollection::storageSizeString() const
    {
        return MongoUtils::buildNiceSizeString(_info.storageSizeBytes());
    }

    void MongoCollection::loadIndexes()
    {
        EventsInfo::LoadCollectionIndexesRequestInfo inf(_info);
        emit startedIndexListLoad(inf);
        _database->server()->postEventToDataBase(new Events::LoadCollectionIndexRequestEvent(this, inf));
    }

    void MongoCollection::createIndex(const EnsureIndex &oldInfo, const EnsureIndex &newInfo)
    {
        EventsInfo::CreateIndexInfo inf(oldInfo, newInfo);
        _database->server()->postEventToDataBase(new Events::CreateIndexRequestEvent(this, inf));
        loadIndexes();
    }

    void MongoCollection::dropIndex(const std::string &indexName)
    {
        EventsInfo::DropIndexInfo inf(_info, indexName);
        _database->server()->postEventToDataBase(new Events::DropIndexRequestEvent(this, inf));
        loadIndexes();
    }

    void MongoCollection::customEvent(QEvent *event)
    {
        QEvent::Type type = event->type();
        if(type==static_cast<QEvent::Type>(Events::LoadCollectionIndexResponceEvent::EventType)){
            Events::LoadCollectionIndexResponceEvent *ev = static_cast<Events::LoadCollectionIndexResponceEvent*>(event);
            Events::LoadCollectionIndexResponceEvent::value_type inf = ev->value();            
            emit finishedIndexListLoad(inf);
        }
        else if(type==static_cast<QEvent::Type>(Events::DropIndexRequestEvent::EventType)){
            Events::DropIndexRequestEvent *ev = static_cast<Events::DropIndexRequestEvent*>(event);
        }
        return BaseClass::customEvent(event);
    }
}
