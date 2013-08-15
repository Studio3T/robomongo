#pragma once

namespace Robomongo
{
    enum UUIDEncoding
    {
        DefaultEncoding = 0,
        JavaLegacy      = 1,
        CSharpLegacy    = 2,
        PythonLegacy    = 3
    };

    enum SupportedTimes
    {
        Utc =0,
        LocalTime=1
    };
}

