#pragma once

namespace Robomongo
{
    namespace HighDpiConstants
    {
        int const WIN_HIGH_DPI_BUTTON_HEIGHT = 23;
        int const MACOS_HIGH_DPI_BUTTON_HEIGHT = 34;
    }

    enum AuthMechanism {
        SCRAM_SHA_1,
        SCRAM_SHA_256,
        MONGODB_CR
    };

    inline AuthMechanism authMechanismFromStr(std::string_view str) {
        if      (str == "SCRAM-SHA-1")   return SCRAM_SHA_1;
        else if (str == "SCRAM-SHA-256") return SCRAM_SHA_256;
        else if (str == "MONGODB-CR")    return MONGODB_CR;
        else                             return SCRAM_SHA_1;
    }

} /* end of Robomongo namespace */