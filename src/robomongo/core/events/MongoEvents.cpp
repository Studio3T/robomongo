#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/Core.h"

namespace Robomongo
{
    R_REGISTER_EVENT(EstablishConnectionRequest)
    R_REGISTER_EVENT(EstablishConnectionResponse)
    R_REGISTER_EVENT(LoadDatabaseNamesRequest)
    R_REGISTER_EVENT(LoadDatabaseNamesResponse)
    R_REGISTER_EVENT(ConnectingEvent)
    R_REGISTER_EVENT(ConnectionFailedEvent)
    R_REGISTER_EVENT(ConnectionEstablishedEvent)
    R_REGISTER_EVENT(DatabaseListLoadedEvent)
    R_REGISTER_EVENT(OpeningShellEvent)
    R_REGISTER_EVENT(ExecuteQueryRequest)
    R_REGISTER_EVENT(ExecuteQueryResponse)
    R_REGISTER_EVENT(DocumentListLoadedEvent)
    R_REGISTER_EVENT(ExecuteScriptRequest)
    R_REGISTER_EVENT(ExecuteScriptResponse)
    R_REGISTER_EVENT(AutocompleteRequest)
    R_REGISTER_EVENT(AutocompleteResponse)
    R_REGISTER_EVENT(ScriptExecutedEvent)
    R_REGISTER_EVENT(ScriptExecutingEvent)
    R_REGISTER_EVENT(InsertDocumentRequest)
    R_REGISTER_EVENT(InsertDocumentResponse)
    R_REGISTER_EVENT(RemoveDocumentRequest)
    R_REGISTER_EVENT(RemoveDocumentResponse)
    R_REGISTER_EVENT(CreateDatabaseRequest)
    R_REGISTER_EVENT(CreateDatabaseResponse)
    R_REGISTER_EVENT(DropDatabaseRequest)
    R_REGISTER_EVENT(DropDatabaseResponse)
    R_REGISTER_EVENT(QueryWidgetUpdatedEvent)
}
