#pragma once

#include <QFrame>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPushButton;
class QToolButton;
class QCheckBox;
QT_END_NAMESPACE
class QsciScintilla;

namespace Robomongo
{
    class FindFrame : public QFrame
    {
        Q_OBJECT
    public:
        enum
        {
            HeightFindPanel = 40
        };
        typedef QFrame BaseClass;
        explicit FindFrame(QWidget *parent);
        QsciScintilla *const sciScintilla() const
        {
            return _scin;
        }

    protected:
        void wheelEvent(QWheelEvent *e);
        void keyPressEvent(QKeyEvent *e);

    private Q_SLOTS:
        void goToNextElement();
        void goToPrevElement();

    private:
        QsciScintilla *const _scin;
        QFrame *const _findPanel;
        QTextEdit *const _findLine;
        QToolButton *const _close;
        QPushButton *const _next;
        QPushButton *const _prev;
        QCheckBox *const  _caseSensitive;
    };
}

