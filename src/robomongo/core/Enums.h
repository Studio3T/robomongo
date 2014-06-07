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
        Utc       = 0,
        LocalTime = 1
    };

    enum ViewMode
    {
        Text = 0,
        Tree = 1,
        Table = 2,
        Custom = 3
    };

    enum AutocompletionMode
    {
        AutocompleteNone = 0,
        AutocompleteAll = 1,
        AutocompleteNoCollectionNames = 2
    };

    const char *convertUUIDEncodingToString(UUIDEncoding uuidCode);
    UUIDEncoding convertStringToUUIDEncoding(const char *text);

    const char *convertTimesToString(SupportedTimes time);
    SupportedTimes convertStringToTimes(const char *text);

    const char *convertViewModeToString(ViewMode mode);
    ViewMode convertStringToViewMode(const char *text);
}

