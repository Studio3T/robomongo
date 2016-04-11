#pragma once

#include <QFrame>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPushButton;
class QToolButton;
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE
class QsciScintilla;

namespace Robomongo
{
    class RoboScintilla;

    class FindFrame : public QFrame
    {
        Q_OBJECT
    public:
        enum
        {
            HeightFindPanel = 38
        };
        typedef QFrame BaseClass;
        explicit FindFrame(QWidget *parent);
        RoboScintilla *const sciScintilla() const
        {
            return _scin;
        }
        void toggleComments();
        virtual ~FindFrame();
    protected:
        virtual void wheelEvent(QWheelEvent *e);
        virtual void keyPressEvent(QKeyEvent *e);

    private Q_SLOTS:
        void goToNextElement();
        void goToPrevElement();

    private:
        void findElement(bool forward);
        void setLineComment(const int lineIndex, const bool commentOut);
        RoboScintilla *const _scin;
        QFrame *const _findPanel;
        QLineEdit *const _findLine;
        QToolButton *const _close;
        QPushButton *const _next;
        QPushButton *const _prev;
        QCheckBox *const  _caseSensitive;
        const char *_commentSign;
        const int _commentSignLength;
        QWidget *_parent;
    };
}

