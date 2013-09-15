#pragma once

#include <QKeyEvent>

namespace Robomongo
{
    namespace KeyboardManager
    {
        bool isNewTabShortcut(QKeyEvent *keyEvent);
        bool isSetFocusOnQueryLineShortcut(QKeyEvent *keyEvent);
        bool isExecuteScriptShortcut(QKeyEvent *keyEvent);
        bool isAutoCompleteShortcut(QKeyEvent *keyEvent);
        bool isHideAutoCompleteShortcut(QKeyEvent *keyEvent);
        bool isNextTabShortcut(QKeyEvent *keyEvent);
        bool isPreviousTabShortcut(QKeyEvent *keyEvent);
    }
}
