#pragma once

#include <QFrame>
QT_BEGIN_NAMESPACE
class QLabel;
class QCompleter;
QT_END_NAMESPACE

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/CursorPosition.h"

namespace Robomongo
{
    class FindFrame;
    class TopStatusBar;
    class MongoShell;
    class Indicator;
    class QueryWidget;

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
        ScriptWidget(MongoShell *shell, QueryWidget* parent);

        /**
         * @reimp
         */
        bool eventFilter(QObject *obj, QEvent *e);

        void setup(const MongoShellExecResult & execResult);
        void setTextCursor(const CursorPosition &cursor = CursorPosition());
        QString text() const;
        QString selectedText() const;
        void selectAll();
        void setScriptFocus();
        void setCurrentDatabase(const std::string &database, bool isValid = true);
        void setCurrentServer(const std::string &address, bool isValid = true);
        void showAutocompletion(const QStringList &list, const QString &prefix);
        void showAutocompletion();
        void hideAutocompletion();
        bool getDisableTextAndCursorNotifications() { return _disableTextAndCursorNotifications; }
        void setDisableTextAndCursorNotifications(const bool value) { _disableTextAndCursorNotifications = value; }

        void disableFixedHeight() const;

    Q_SIGNALS:
        void textChanged();

    public Q_SLOTS:
        void setText(const QString &text);
        void ui_queryLinesCountChanged();

    private Q_SLOTS:
        void onTextChanged();
        void onCursorPositionChanged(int line, int index);
        void onCompletionActivated(const QString&);

    private:
        void configureQueryText();

        /**
         * @brief Calculates line height of text editor
         */
        int lineHeight() const;

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
        int editorHeight(int lines) const;
        
        AutoCompletionInfo sanitizeForAutocompletion();
        FindFrame *_queryText;
        TopStatusBar *_topStatusBar;
        QCompleter *_completer;
        MongoShell *_shell;
        AutoCompletionInfo _currentAutoCompletionInfo;

        QueryWidget *_parent;

        bool _textChanged;
        bool _disableTextAndCursorNotifications;
    };

    class TopStatusBar : public QFrame
    {
        Q_OBJECT

    public:
        TopStatusBar(const std::string &connectionName, const std::string &serverName, const std::string &dbName);
        void setCurrentDatabase(const std::string &database, bool isValid = true);
        void setCurrentServer(const std::string &address, bool isValid = true);
        void showProgress();
        void hideProgress();

    private:
        Indicator *_currentDatabaseLabel;
        Indicator *_currentServerLabel;
        Indicator *_currentConnectionLabel;
        QColor _textColor;
    };
}
