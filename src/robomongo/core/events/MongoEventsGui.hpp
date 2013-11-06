#pragma once

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/domain/MongoNamespace.h"
#include <mongo/bson/bsonobj.h>

namespace Robomongo
{
    struct SaveObjectInfo 
    {
        mongo::BSONObj _obj;
        MongoNamespace _ns;
        bool _overwrite;
    };

    struct SaveObjectEvent 
        : public QtUtils::Event<SaveObjectEvent, SaveObjectInfo>
    {
        typedef QtUtils::Event<SaveObjectEvent, SaveObjectInfo> BaseClass;
        SaveObjectEvent(QObject *const sender, const BaseClass::value_type &val, ErrorInfo er = ErrorInfo())
            : BaseClass(sender, val, er) {}
    };
}
