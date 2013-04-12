#pragma once

#include <QObject>
#include <QKeyEvent>

namespace Robomongo
{
    class KeyboardManager : public QObject
    {
        Q_OBJECT

    public:
        explicit KeyboardManager(QObject *parent = 0);
        bool isNewTabShortcut(QKeyEvent *keyEvent);
        bool isSetFocusOnQueryLineShortcut(QKeyEvent *keyEvent);
        bool isExecuteScriptShortcut(QKeyEvent *keyEvent);
        bool isAutoCompleteShortcut(QKeyEvent *keyEvent);
        bool isHideAutoCompleteShortcut(QKeyEvent *keyEvent);
        bool isNextTabShortcut(QKeyEvent *keyEvent);
        bool isPreviousTabShortcut(QKeyEvent *keyEvent);
    };
}
