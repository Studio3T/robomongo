#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/Core.h"

namespace Robomongo
{
    R_REGISTER_EVENT(ConnectingEvent)
    R_REGISTER_EVENT(ConnectionFailedEvent)
    R_REGISTER_EVENT(ConnectionEstablishedEvent)
    R_REGISTER_EVENT(DatabaseListLoadedEvent)
    R_REGISTER_EVENT(OpeningShellEvent)
    R_REGISTER_EVENT(DocumentListLoadedEvent)
    R_REGISTER_EVENT(ScriptExecutedEvent)
    R_REGISTER_EVENT(ScriptExecutingEvent)
    R_REGISTER_EVENT(QueryWidgetUpdatedEvent)
}
