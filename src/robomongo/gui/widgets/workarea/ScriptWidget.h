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

    class ScriptWidget : public QFrame
    {
        Q_OBJECT

    public:
        ScriptWidget(MongoShell *shell);

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
        void showAutocompletion(const QStringList &list);

        TopStatusBar *statusBar() const { return _topStatusBar; }

    private slots:
        void ui_queryLinesCountChanged();
        void onTextChanged();
        void onUserListActivated(int, QString);

    private:
        void _configureQueryText();
        RoboScintilla *_queryText;
        TopStatusBar *_topStatusBar;
        QCompleter *_completer;
        MongoShell *_shell;
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
