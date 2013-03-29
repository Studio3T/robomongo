#pragma once

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QCompleter>

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/CursorPosition.h"

namespace Robomongo
{
    class RoboScintilla;
    class TopStatusBar;
    class MongoShell;

    class AutoCompletionInfo
    {
    public:
        AutoCompletionInfo() :
            _text(""),
            _line(0),
            _lineIndexLeft(0),
            _lineIndexRight(0) {}

        AutoCompletionInfo(const QString &text, int line, int lineIndexLeft, int lineIndexRight = 0) :
            _text(text),
            _line(line),
            _lineIndexLeft(lineIndexLeft),
            _lineIndexRight(lineIndexRight) {}

        QString text() const { return _text; }
        int line() const { return _line; }
        int lineIndexLeft() const { return _lineIndexLeft; }
        int lineIndexRight() const { return _lineIndexRight; }
        bool isEmpty() const { return _text.isEmpty(); }

    private:
        QString _text;      // text, for which we are trying to find completions
        int _line;          // line number in editor, where 'text' is located
        int _lineIndexLeft; // index of first char in the line, where 'text' is started
        int _lineIndexRight;// index of last char in the line, where 'text' is ended
    };

    class ScriptWidget : public QFrame
    {
        Q_OBJECT

    public:
        ScriptWidget(MongoShell *shell);

        /**
         * @reimp
         */
        bool eventFilter(QObject *obj, QEvent *e);

        void setup(const MongoShellExecResult & execResult);
        void setText(const QString &text);
        void setTextCursor(const CursorPosition &cursor = CursorPosition());
        QString text() const;
        QString selectedText() const;
        void selectAll();
        void setScriptFocus();
        void setCurrentDatabase(const QString &database, bool isValid = true);
        void setCurrentServer(const QString &address, bool isValid = true);
        void showProgress();
        void hideProgress();
        void showAutocompletion(const QStringList &list, const QString &prefix);
        void showAutocompletion();
        void hideAutocompletion();

        TopStatusBar *statusBar() const { return _topStatusBar; }

    private slots:
        void ui_queryLinesCountChanged();
        void onTextChanged();
        void onCursorPositionChanged(int line, int index);
        void onCompletionActivated(QString);

    private:
        void _configureQueryText();
        QFont chooseTextFont();

        /**
         * @brief Calculates line height of text editor
         */
        int lineHeight();

        /**
         * @brief Calculates char width of text editor
         */
        int charWidth();

        /**
         * @brief Because of different fonts, differents OSes etc. we didn't find
         * a better way to find required position for autocompletion box.
         * We just hardcoded it.
         */
        int autocompletionBoxLeftPosition();

        /**
         * @brief Calculates preferable editor height for specified number of lines
         */
        int editorHeight(int lines);

        /**
         * @brief Checks whether 'ch' is a stop char
         * @param direction: true for right direction, false for left direction
         */
        bool isStopChar(const QChar &ch, bool direction);

        bool isForbiddenChar(const QChar &ch);

        AutoCompletionInfo sanitizeForAutocompletion();
        RoboScintilla *_queryText;
        TopStatusBar *_topStatusBar;
        QCompleter *_completer;
        MongoShell *_shell;
        QFont _textFont;
        AutoCompletionInfo _currentAutoCompletionInfo;
        bool _textChanged;
        bool _disableTextAndCursorNotifications;
    };

    class ElidedLabel : public QLabel
    {
        Q_OBJECT

    public:
        ElidedLabel(){}
        ElidedLabel(const QString &text) : QLabel(text) { }
        QSize minimumSizeHint() const;
        QSize sizeHint() const;

    protected:
        void paintEvent(QPaintEvent *event);
    };

    class TopStatusBar : public QFrame
    {
        Q_OBJECT

    public:
        TopStatusBar(MongoShell *shell);
        void setCurrentDatabase(const QString &database, bool isValid = true);
        void setCurrentServer(const QString &address, bool isValid = true);
        void showProgress();
        void hideProgress();

    private:
        QLabel *_currentDatabaseLabel;
        QLabel *_currentServerLabel;
        QLabel *_progressLabel;
        MongoShell *_shell;
        QColor _textColor;
    };
}
