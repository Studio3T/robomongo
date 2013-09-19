#include "robomongo/core/EventError.h"

namespace Robomongo
{
    EventError::EventError() {}

    EventError::EventError(const std::string &errorMessage) :
        _errorMessage(errorMessage) {}

    bool EventError::isNull() const { return _errorMessage.empty(); }

    const std::string &EventError::errorMessage() const { return _errorMessage; }
}
