#include "robomongo/core/Enums.h"
#include <string.h>

namespace
{
    const char *viewModeAsoc[Robomongo::Custom+1] = {"Text mode", "Tree mode", "Table mode", "Custom mode"};
    const char *timesAsoc[Robomongo::LocalTime+1] = {"UTC", "Local Timezone"};
    const char *uuidAsoc[Robomongo::PythonLegacy+1] = {"Default encoding", "Java encoding", "CSharp encoding", "Python encoding"};

    template<typename type, int size>
    inline type findTypeInArray(const char *(&arr)[size], const char *text)
    {
        for (int i = 0; i < size; ++i )
        {
            if (strcmp(text, arr[i]) == 0)
            {
                return static_cast<type>(i);
            }
        }
        return static_cast<type>(0);
    }
}

namespace Robomongo
{
    const char *convertUUIDEncodingToString(UUIDEncoding uuidCode)
    {
       return uuidAsoc[uuidCode];
    }

    UUIDEncoding convertStringToUUIDEncoding(const char *text)
    {
        return findTypeInArray<UUIDEncoding>(uuidAsoc, text);
    }

    const char *convertTimesToString(SupportedTimes time)
    {
        return timesAsoc[time];
    }

    SupportedTimes convertStringToTimes(const char *text)
    {
        return findTypeInArray<SupportedTimes>(timesAsoc, text);
    }

    const char *convertViewModeToString(ViewMode mode)
    {
        return viewModeAsoc[mode];
    }

    ViewMode convertStringToViewMode(const char *text)
    {
        return findTypeInArray<ViewMode>(viewModeAsoc, text);
    }
}

