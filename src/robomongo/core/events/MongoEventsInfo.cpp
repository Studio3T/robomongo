#include "robomongo/core/events/MongoEventsInfo.h"

namespace Robomongo
{
    EnsureIndexInfo::EnsureIndexInfo(const MongoCollectionInfo &collection, const QString &name, const QString &request,
        bool isUnique, bool isBackGround, bool isDropDuplicates,bool isSparce,const QString &expireAfter,
        const QString &defaultLanguage,const QString &languageOverride,const QString &textWeights) :
    _name(name),
        _collection(collection),
        _request(request),
        _isUnique(isUnique),
        _isBackGround(isBackGround),
        _isDropDuplicates(isDropDuplicates){}
}