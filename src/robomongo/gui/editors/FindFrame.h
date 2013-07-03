#pragma once
#include <QFrame>
QT_BEGIN_NAMESPACE
class QTextEdit;
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE
class QsciScintilla;
namespace Robomongo
{
    class FindFrame
            : public QFrame
    {
        Q_OBJECT
    public:
        enum
        {
            height_find_panel = 40
        };
        typedef QFrame base_class;
        explicit FindFrame(QWidget *parent);
        QsciScintilla * const sciScintilla()const
        {
            return scin_;
        }
    protected:
        void wheelEvent(QWheelEvent *e);
        void keyPressEvent(QKeyEvent *e);
    private Q_SLOTS:
        void GoToNextElement();
        void GoToPrevElement();
    private:
        QsciScintilla * const scin_;
        QFrame * const find_panel_;
        QTextEdit *const find_line_;
        QPushButton *const close_;
        QPushButton *const next_;
        QPushButton *const prev_;
        QCheckBox *const  case_sens_;
    };
}

