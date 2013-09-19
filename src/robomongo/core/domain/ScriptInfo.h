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
                   const QString &title = QString(), const QString &filePath = QString());

        bool execute() const { return _execute; }
        QString script() const { return _script; }
        const QString &title() const { return _title; }
        const CursorPosition &cursor() const { return _cursor; }
        void setScript(const QString &script) { _script = script; }
        QString filePath() const { return _filePath; }
        bool loadFromFile(const QString &filePath);
        bool loadFromFile();
        bool saveToFileAs();
        bool saveToFile();

    private:
        QString _script;
        const bool _execute;
        const QString _title;
        const CursorPosition _cursor;
        QString _filePath;
    };
}
