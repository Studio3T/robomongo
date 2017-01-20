#include "robomongo/core/EventError.h"

#include "robomongo/utils/string_operations.h"

namespace Robomongo
{
    EventError::EventError() :
        _isNull(true) {}

    EventError::EventError(const std::string &errorMessage, ErrorCode errorCode /*= Unknown*/,
                           bool showErrorWindow /* = true */) :
        _errorMessage(captilizeFirstChar(errorMessage)),
        _errorCode(errorCode),
        _showErrorWindow(showErrorWindow),
        _isNull(false) {}

    EventError::EventError(const std::string &errorMessage, ReplicaSet const& replicaSetInfo, 
                           bool showErrorWindow /* = true */ ) :
        _errorMessage(errorMessage), 
        _errorCode(SetPrimaryUnreachable),
        _replicaSetInfo(replicaSetInfo),
        _showErrorWindow(showErrorWindow),
        _isNull(false) {}

    bool EventError::isNull() const
    {
        return _isNull;
    }

    const std::string &EventError::errorMessage() const 
    {
        return _errorMessage;
    }

}
