#include "robomongo/core/EventError.h"


namespace Robomongo
{
    // Capitalize first char. (Mongo errors often come all lower case)
    std::string captilizeFirstChar(std::string str);    // todo: move to utils header

    EventError::EventError() :
        _isNull(true) {}

    EventError::EventError(const std::string &errorMessage, ErrorCode errorCode /*= Unknown*/,
                           bool showErrorWindow /* = true */) :
        _errorMessage(captilizeFirstChar(errorMessage)),
        _showErrorWindow(showErrorWindow),
        _isNull(false) {}

    EventError::EventError(const std::string &errorMessage, ReplicaSet replicaSetInfo, 
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

    std::string captilizeFirstChar(std::string str)
    {
        if (!str.empty())
            str[0] = static_cast<char>(toupper(str[0]));

        return str;
    }
}
