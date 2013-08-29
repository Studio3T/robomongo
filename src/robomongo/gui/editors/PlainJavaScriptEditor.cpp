#include "robomongo/gui/editors/PlainJavaScriptEditor.h"

#include <QPainter>
#include <QApplication>
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/editors/jsedit.h"

namespace
{
    int getNumberOfDigits(int x);
}

namespace Robomongo
{
    const QColor RoboScintilla::marginsBackgroundColor = QColor(73, 76, 78);
    const QColor RoboScintilla::caretForegroundColor = QColor("#FFFFFF");
    const QColor RoboScintilla::matchedBraceForegroundColor = QColor("#FF8861");

    RoboScintilla::RoboScintilla(QWidget *parent) : QsciScintilla(parent),
        _ignoreEnterKey(false),
        _ignoreTabKey(false),
        _lineNumberMarginWidth(0),
        _lineNumberDigitWidth(0)
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
        QFont ourFont = GuiRegistry::instance().font();
        setMarginsFont(ourFont);
        setMarginLineNumbers(0, true);
        setMarginsBackgroundColor(QColor(53, 56, 58));
        setMarginsForegroundColor(QColor(173, 176, 178));

        SendScintilla(QsciScintilla::SCI_STYLESETFONT, 1, ourFont.family().data());
        SendScintilla(QsciScintilla::SCI_SETHSCROLLBAR, 0);

        setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); 
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); 

        // Cache width of one digit
        _lineNumberDigitWidth = textWidth(STYLE_LINENUMBER, "0");

        updateLineNumbersMarginWidth();

        connect(this, SIGNAL(linesChanged()), this, SLOT(updateLineNumbersMarginWidth()));
    }

    int RoboScintilla::lineNumberMarginWidth()
    {
        return marginWidth(0);
    }

    int RoboScintilla::textWidth(int style, const QString &text)
    {
        const char *byteArray = (text.toUtf8()).constData();
        return SendScintilla(SCI_TEXTWIDTH, style, byteArray);
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

    void RoboScintilla::showOrHideLinesNumbers()
    {
        if (!lineNumberMarginWidth()) {
            setMarginWidth(0, _lineNumberMarginWidth);
        }
        else {
            setMarginWidth(0, 0);
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

        if(keyEvent->key() == Qt::Key_F11) {
            keyEvent->ignore();
            showOrHideLinesNumbers();
            return;
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

    void RoboScintilla::updateLineNumbersMarginWidth()
    {
        int numberOfDigits = getNumberOfDigits(lines());
        _lineNumberMarginWidth = (numberOfDigits+1) * rowNumberWidth;

        // If line numbers margin already displayed, update its width
        if (lineNumberMarginWidth()) {
            setMarginWidth(0, _lineNumberMarginWidth);
        }
    }
}

namespace
{
    /**
     * @brief Returns the number of digits in an 32-bit integer
     * http://stackoverflow.com/questions/1489830/efficient-way-to-determine-number-of-digits-in-an-integer
     */
    int getNumberOfDigits(int x)
    {
        if (x < 0) return getNumberOfDigits(-x) + 1;

        if (x >= 10000) {
            if (x >= 10000000) {
                if (x >= 100000000) {
                    if (x >= 1000000000)
                        return 10;
                    return 9;
                }
                return 8;
            }
            if (x >= 100000) {
                if (x >= 1000000)
                    return 7;
                return 6;
            }
            return 5;
        }
        if (x >= 100) {
            if (x >= 1000)
                return 4;
            return 3;
        }
        if (x >= 10)
            return 2;
        return 1;
    }
}
