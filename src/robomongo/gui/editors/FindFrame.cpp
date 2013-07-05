#include "robomongo/gui/editors/FindFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QToolButton>
#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/editors/PlainJavaScriptEditor.h"

namespace Robomongo
{
    FindFrame::FindFrame(QWidget *parent) : BaseClass(parent),
        _scin(new RoboScintilla(this)),
        _findPanel(new QFrame(this)),
        _close(new QToolButton(this)),
        _findLine(new QTextEdit(this)),
        _next(new QPushButton("next", this)),
        _prev(new QPushButton("prev", this)),
        _caseSensitive(new QCheckBox("Match case", this))
    {
        _close->setIcon(QIcon(":/robomongo/icons/close_2_16x16.png"));
        _close->setToolButtonStyle(Qt::ToolButtonIconOnly);
        _close->setIconSize(QSize(16, 16));
        _findLine->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);

        QVBoxLayout *main_layout = new QVBoxLayout();
        main_layout->addWidget(_scin);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(0);
        _findPanel->setMaximumHeight(HeightFindPanel);
        _findPanel->setMinimumHeight(HeightFindPanel);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(_close);
        layout->addWidget(_findLine);
        layout->addWidget(_next);
        layout->addWidget(_prev);
        layout->addWidget(_caseSensitive);

        _findPanel->setLayout(layout);
        main_layout->addWidget(_findPanel);
        setLayout(main_layout);
        _findPanel->hide();

        connect(_close, SIGNAL(clicked()), _findPanel, SLOT(hide()));
        connect(_next, SIGNAL(clicked()), this, SLOT(goToNextElement()));
        connect(_prev, SIGNAL(clicked()), this, SLOT(goToPrevElement()));
    }

    void FindFrame::wheelEvent(QWheelEvent *e)
    {
        return BaseClass::wheelEvent(e);
    }

    void FindFrame::keyPressEvent(QKeyEvent *keyEvent)
    {
        bool isFocusScin = _scin->isActiveWindow();

        if(Qt::Key_Escape == keyEvent->key() && isFocusScin) {
            _findPanel->hide();
            _scin->setFocus();
            return keyEvent->accept();
        } else if(((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key()==Qt::Key_F) && isFocusScin) {
            _findPanel->show();
            _findLine->setFocus();
            return keyEvent->accept();
        } else {
            return BaseClass::keyPressEvent(keyEvent);
        }
    }

    void FindFrame::goToNextElement()
    {
        const QString &text = _findLine->toPlainText();
        if(!text.isEmpty())
        {
            bool re = false;
            bool wo = false;
            bool wrap = false;
            int index = 0;
            int line = 0;
            _scin->getCursorPosition(&line, &index);
            _scin->findFirst(text, re, _caseSensitive->checkState() == Qt::Checked, wo, wrap, true, line, index);
        }
    }

    void FindFrame::goToPrevElement()
    {
        const QString &text = _findLine->toPlainText();
        if(!text.isEmpty())
        {
            bool re = false;
            bool wo = false;
            bool wrap = false;
            int index = 0;
            int line = 0;
            _scin->getCursorPosition(&line, &index);
            index -= _scin->selectedText().length();
            _scin->findFirst(text, re, _caseSensitive->checkState() == Qt::Checked, wo, wrap, false, line, index);
        }
    }
}
