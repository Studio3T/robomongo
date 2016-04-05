#include "robomongo/core/EventError.h"

namespace Robomongo
{
    EventError::EventError() :
        _isNull(true) {}

    EventError::EventError(const std::string &errorMessage) :
        _errorMessage(errorMessage),
        _isNull(false) {}

    bool EventError::isNull() const {
        return _isNull;
    }

    const std::string &EventError::errorMessage() const {
        return _errorMessage;
    }
}
