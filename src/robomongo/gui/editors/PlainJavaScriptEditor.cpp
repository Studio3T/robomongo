#include "robomongo/gui/editors/PlainJavaScriptEditor.h"

#include <QPainter>
#include <QApplication>

#include "robomongo/gui/editors/jsedit.h"

namespace Robomongo
{
    const QColor RoboScintilla::marginsBackgroundColor = QColor(73, 76, 78);
    const QColor RoboScintilla::caretForegroundColor=QColor("#FFFFFF");
    const QColor RoboScintilla::matchedBraceForegroundColor=QColor("#FF8861");
    RoboScintilla::RoboScintilla(QWidget *parent) : QsciScintilla(parent),
        _ignoreEnterKey(false),
        _ignoreTabKey(false)
    {
        setAutoIndent(true);
        setIndentationsUseTabs(false);
        setIndentationWidth(indentationWidth);
        setUtf8(true);
        setMarginWidth(1, 0);

        setCaretForegroundColor(caretForegroundColor);
        setMatchedBraceForegroundColor(matchedBraceForegroundColor); //1AB0A6
        setMatchedBraceBackgroundColor(marginsBackgroundColor);
        setContentsMargins(0, 0, 0, 0);
        setViewportMargins(3, 3, 3, 3);

        // Margin 0 is used for line numbers
        QFontMetrics fontmetrics = QFontMetrics(font());
        setMarginsFont(font());
        setMarginWidth(0, fontmetrics.width("00000") + rowNumberWidth);
        setMarginLineNumbers(0, true);
        setMarginsBackgroundColor(marginsBackgroundColor);

        SendScintilla(QsciScintilla::SCI_STYLESETFONT, 1, font().family().data() );
        SendScintilla(QsciScintilla::SCI_SETHSCROLLBAR, 0);
    }
    void RoboScintilla::wheelEvent(QWheelEvent *e)
    {
        if (this->isActiveWindow()) {
            QsciScintilla::wheelEvent(e);
        }
        else {
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
            BaseClass::keyPressEvent(keyEvent);
        }
    }
}
