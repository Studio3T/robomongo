#pragma once

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

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

        void setText(const QString &text);
        QString text() const;
        QString selectedText() const;
        void selectAll();
        void setScriptFocus();

        TopStatusBar *statusBar() const { return _topStatusBar; }


    private slots:
        void ui_queryLinesCountChanged();

    private:

        void _configureQueryText();
        /*
        ** Query text
        */
        RoboScintilla *_queryText;
        TopStatusBar *_topStatusBar;
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

    private:
        QLabel *_currentDatabaseLabel;
        QLabel *_currentServerLabel;
        MongoShell *_shell;
        QColor _textColor;
    };
}
