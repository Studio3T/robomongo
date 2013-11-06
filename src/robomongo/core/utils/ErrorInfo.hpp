#pragma once
#include <string>

namespace Robomongo
{
    struct ErrorInfo 
    {
        enum ErrorsType { _NONE, _EXCEPTION, _ERROR };
        ErrorInfo():_description(),_errorType(_NONE){}
        ErrorInfo(const std::string &desc, ErrorsType errorType):_description(desc),_errorType(errorType) {}
        bool isError() const { return _errorType != _NONE; }
        std::string _description;
        ErrorsType _errorType;
    };
}
