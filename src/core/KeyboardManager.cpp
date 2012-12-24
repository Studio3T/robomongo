#include "KeyboardManager.h"

using namespace Robomongo;

KeyboardManager::KeyboardManager(QObject *parent) :
    QObject(parent)
{
}

bool KeyboardManager::isNewTabShortcut(QKeyEvent *keyEvent)
{
    bool ctrlShiftReturn = (keyEvent->modifiers() & Qt::ControlModifier) &&
                           (keyEvent->modifiers() & Qt::ShiftModifier) &&
                           (keyEvent->key()==Qt::Key_Return || keyEvent->key()==Qt::Key_Enter);

    bool ctrlT = (keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key()==Qt::Key_T);

    return ctrlShiftReturn || ctrlT;
}

bool KeyboardManager::isSetFocusOnQueryLineShortcut(QKeyEvent *keyEvent)
{
    return keyEvent->key() == Qt::Key_F6;
}

bool KeyboardManager::isExecuteScriptShortcut(QKeyEvent *keyEvent)
{
    return (keyEvent->modifiers() & Qt::ControlModifier)
            &&
           (keyEvent->key()==Qt::Key_Return || keyEvent->key()==Qt::Key_Enter);
}
