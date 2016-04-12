#pragma once

#include <Qsci/qsciscintilla.h>

namespace Robomongo
{
    class RoboScintilla : public QsciScintilla
    {
        Q_OBJECT
    public:
        typedef QsciScintilla BaseClass;
        enum { rowNumberWidth = 6, indentationWidth = 4 };
        static const QColor marginsBackgroundColor;
        static const QColor caretForegroundColor;
        static const QColor matchedBraceForegroundColor;

        RoboScintilla(QWidget *parent = NULL);
        void setIgnoreEnterKey(bool ignore) { _ignoreEnterKey = ignore; }
        void setIgnoreTabKey(bool ignore) { _ignoreTabKey = ignore; }
        int lineNumberMarginWidth() const;
        int textWidth(int style, const QString &text);
        void setAppropriateBraceMatching();

    protected:
        void wheelEvent(QWheelEvent *e);
        void keyPressEvent(QKeyEvent *e);

    private Q_SLOTS:
        void updateLineNumbersMarginWidth();

    private:
        void setLineNumbers(bool displayNumbers);
        void toggleLineNumbers();
        bool _ignoreEnterKey;
        bool _ignoreTabKey;
        int _lineNumberMarginWidth;
        int _lineNumberDigitWidth;
    };
}
