#include "robomongo/utils/string_operations.h"

namespace Robomongo
{
    std::string captilizeFirstChar(std::string str)
    {
        if (!str.empty())
            str[0] = static_cast<char>(toupper(str[0]));

        return str;
    }

}   // end of name space Robomongo