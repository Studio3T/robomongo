#ifndef KEYBOARDMANAGER_H
#define KEYBOARDMANAGER_H

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
    };
}


#endif // KEYBOARDMANAGER_H
