#pragma once
#include <QString>
#include "robomongo/core/domain/CursorPosition.h"

namespace Robomongo
{
    class ScriptInfo
    {
    public:
         ScriptInfo(const QString &script, bool execute = false,
                   const CursorPosition &position = CursorPosition(),
                   const QString &title = QString());

        bool execute() const { return _execute; }
        QString script() const { return _script; }
        QString title() const { return _title; }
        CursorPosition cursor() const { return _cursor; }

    private:
        bool _execute;
        QString _script;
        QString _title;
        CursorPosition _cursor;
    };
}
