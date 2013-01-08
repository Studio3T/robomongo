#include "ImplicitlyShared.h"

using namespace Robomongo;

/**
 * Creates ConnectionSettings with default values
 */
ImplicitlyShared::ImplicitlyShared() : _data(new ImplicitlySharedPrivate)
{
    _data->id = 0;
    _data->serverPort = 27017;
}

