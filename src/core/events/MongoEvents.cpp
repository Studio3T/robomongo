#include "MongoEvents.h"

using namespace Robomongo;

const QEvent::Type CollectionNamesLoaded::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());
const QEvent::Type DatabaseNamesLoaded::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());
const QEvent::Type ConnectionEstablished::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());
const QEvent::Type ConnectionFailed::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());
