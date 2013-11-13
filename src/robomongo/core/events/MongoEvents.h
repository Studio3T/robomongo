#pragma once

#include <QString>
#include <QStringList>
#include <QEvent>
#include <mongo/client/dbclientinterface.h>

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/CursorPosition.h"
#include "robomongo/core/domain/MongoUser.h"
#include "robomongo/core/domain/MongoFunction.h"
#include "robomongo/core/events/MongoEventsInfo.h"
#include "robomongo/core/Event.h"

namespace Robomongo
{
    class MongoDatabase;
    class MongoServer;
}
