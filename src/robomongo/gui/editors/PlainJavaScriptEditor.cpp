#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include <QPainter>
#include <QApplication>
#include "robomongo/gui/editors/jsedit.h"

namespace Robomongo
{
    RoboScintilla::RoboScintilla(QWidget *parent) : QsciScintilla(parent),
        _ignoreEnterKey(false), _ignoreTabKey(false)
    {
        setContentsMargins(0, 0, 0, 0);
        setViewportMargins(3,3,3,3);
    }

    void RoboScintilla::wheelEvent(QWheelEvent *e)
    {
        if (this->isActiveWindow()){
            QsciScintilla::wheelEvent(e);
        }
        else
        {
            qApp->sendEvent(parentWidget(), e);
            e->accept();
        }
    }

    void RoboScintilla::keyPressEvent(QKeyEvent *keyEvent)
    {
        if (_ignoreEnterKey) {
            if (keyEvent->key() == Qt::Key_Return) {
                keyEvent->ignore();
                _ignoreEnterKey = false;
                return;
            }
        }

        if (_ignoreTabKey) {
            if (keyEvent->key() == Qt::Key_Tab) {
                keyEvent->ignore();
                _ignoreTabKey = false;
                return;
            }
        }

        if (((keyEvent->modifiers() & Qt::ControlModifier) &&
            (keyEvent->key()==Qt::Key_F4 || keyEvent->key()==Qt::Key_W ||
             keyEvent->key()==Qt::Key_T || keyEvent->key()==Qt::Key_Space))
            || keyEvent->key() == Qt::Key_Escape /*|| keyEvent->key() == Qt::Key_Return*/
            || ((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key()==Qt::Key_F)
            || ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->modifiers() & Qt::AltModifier) && keyEvent->key()==Qt::Key_Left)
            || ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->modifiers() & Qt::AltModifier) && keyEvent->key()==Qt::Key_Right)
           )
        {
            keyEvent->ignore();
        }
        else
        {
            base_class::keyPressEvent(keyEvent);
        }
    }
}
