#include "robomongo/core/KeyboardManager.h"

namespace Robomongo
{
    bool KeyboardManager::isNewTabShortcut(QKeyEvent *keyEvent)
    {
        bool ctrlShiftReturn = (keyEvent->modifiers() & Qt::ControlModifier) &&
            (keyEvent->modifiers() & Qt::ShiftModifier) &&
            (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter);

        // TODO: compare with QKeySequence::AddTab rather than Ctrl+T
        bool ctrlT = (keyEvent->modifiers() & Qt::ControlModifier) && !(keyEvent->modifiers() & Qt::ShiftModifier) && (keyEvent->key() == Qt::Key_T);

        return ctrlShiftReturn || ctrlT;
    }

    bool KeyboardManager::isDuplicateTabShortcut(QKeyEvent *keyEvent)
    {
        bool ctrlShiftT = (keyEvent->modifiers() & Qt::ControlModifier) &&
            (keyEvent->modifiers() & Qt::ShiftModifier) &&
            (keyEvent->key() == Qt::Key_T);

        return ctrlShiftT;
    }

    bool KeyboardManager::isSetFocusOnQueryLineShortcut(QKeyEvent *keyEvent)
    {
        return keyEvent->key() == Qt::Key_F6;
    }

    bool KeyboardManager::isExecuteScriptShortcut(QKeyEvent *keyEvent)
    {
        return (keyEvent->modifiers() & Qt::ControlModifier)
            &&
            (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter);
    }

    bool KeyboardManager::isAutoCompleteShortcut(QKeyEvent *keyEvent)
    {
        return (keyEvent->modifiers() & Qt::ControlModifier)
            &&
            (keyEvent->key() == Qt::Key_Space);
    }

    bool KeyboardManager::isHideAutoCompleteShortcut(QKeyEvent *keyEvent)
    {
        return (keyEvent->key() == Qt::Key_Escape);
    }

    bool KeyboardManager::isNextTabShortcut(QKeyEvent *keyEvent)
    {
        return (keyEvent->modifiers() & Qt::ControlModifier)
            && (keyEvent->modifiers() & Qt::AltModifier)
            && (keyEvent->key() == Qt::Key_Right);
    }

    bool KeyboardManager::isPreviousTabShortcut(QKeyEvent *keyEvent)
    {
        return (keyEvent->modifiers() & Qt::ControlModifier)
            && (keyEvent->modifiers() & Qt::AltModifier)
            && (keyEvent->key() == Qt::Key_Left);
    }
    
    bool KeyboardManager::isToggleCommentsShortcut(QKeyEvent *keyEvent)
    {
        return ((keyEvent->modifiers() & Qt::ControlModifier)
               && (keyEvent->key() == Qt::Key_Slash))
            ||
               ((keyEvent->modifiers() & Qt::ControlModifier)
               && (keyEvent->modifiers() & Qt::ShiftModifier)
               && (keyEvent->key() == Qt::Key_C));
    }
}
