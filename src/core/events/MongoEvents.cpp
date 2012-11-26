#include "MongoEvents.h"

using namespace Robomongo;

#define R_REGISTER_EVENT_TYPE(EVENT_TYPE) \
    const QEvent::Type EVENT_TYPE::EventType = static_cast<QEvent::Type>(QEvent::registerEventType())

R_REGISTER_EVENT_TYPE(EstablishConnectionRequest);
R_REGISTER_EVENT_TYPE(EstablishConnectionResponse);
R_REGISTER_EVENT_TYPE(LoadDatabaseNamesRequest);
R_REGISTER_EVENT_TYPE(LoadDatabaseNamesResponse);
R_REGISTER_EVENT_TYPE(LoadCollectionNamesRequest);
R_REGISTER_EVENT_TYPE(LoadCollectionNamesResponse);
R_REGISTER_EVENT_TYPE(SomethingHappened);
R_REGISTER_EVENT_TYPE(ConnectingEvent);
R_REGISTER_EVENT_TYPE(ConnectionFailedEvent);
R_REGISTER_EVENT_TYPE(ConnectionEstablishedEvent);
R_REGISTER_EVENT_TYPE(DatabaseListLoadedEvent);
R_REGISTER_EVENT_TYPE(CollectionListLoadedEvent);
R_REGISTER_EVENT_TYPE(OpeningShellEvent);

