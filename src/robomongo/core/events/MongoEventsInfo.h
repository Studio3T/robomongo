#pragma once
#include <QString>
#include "robomongo/core/domain/MongoCollectionInfo.h"

namespace Robomongo
{
    struct EnsureIndexInfo
    {
        EnsureIndexInfo(const MongoCollectionInfo &collection, const QString &name=QString(), const QString &request=QString(),
            bool isUnique=false, bool isBackGround=false, bool isDropDuplicates=false, bool isSparce=false, int expireAfter=0,
            const QString &defaultLanguage=QString(), const QString &languageOverride=QString(), const QString &textWeights=QString());

        MongoCollectionInfo _collection;
        QString _name;
        QString _request;
        bool _isUnique;
        bool _isBackGround;
        bool _isDropDuplicates;
        bool _isSparce;
        int _expireAfter;
        QString _defaultLanguage;
        QString _languageOverride;
        QString _textWeights;
    };
}
