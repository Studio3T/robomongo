#include "ImplicitlyShared.h"

using namespace Robomongo;

/**
 * Creates ConnectionRecord with default values
 */
ImplicitlyShared::ImplicitlyShared() : _data(new ImplicitlySharedPrivate)
{
    _data->id = 0;
    _data->databasePort = 27017;
}

