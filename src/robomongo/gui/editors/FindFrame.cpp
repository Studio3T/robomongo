#include "robomongo/gui/editors/FindFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QToolButton>
#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/editors/PlainJavaScriptEditor.h"

namespace Robomongo
{
    FindFrame::FindFrame(QWidget *parent) : BaseClass(parent),
        _scin(new RoboScintilla()),
        _findPanel(new QFrame(this)),
        _close(new QToolButton(this)),
        _findLine(new QLineEdit(this)),
        _next(new QPushButton("Next", this)),
        _prev(new QPushButton("Previous", this)),
        _caseSensitive(new QCheckBox("Match case", this))
    {
        _close->setIcon(QIcon(":/robomongo/icons/close_2_16x16.png"));
        _close->setToolButtonStyle(Qt::ToolButtonIconOnly);
        _close->setIconSize(QSize(16, 16));
        _findLine->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);

        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(6, 0, 6, 0);
        layout->setSpacing(4);
        layout->addWidget(_close);
        layout->addWidget(_findLine);
        layout->addWidget(_next);
        layout->addWidget(_prev);
        layout->addWidget(_caseSensitive);

        _findPanel->setMaximumHeight(HeightFindPanel);
        _findPanel->setMinimumHeight(HeightFindPanel);
        _findPanel->setLayout(layout);

        QVBoxLayout *mainLayout = new QVBoxLayout();
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->addWidget(_scin);
        mainLayout->addWidget(_findPanel);
        setLayout(mainLayout);

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
        } else if (Qt::Key_Return == keyEvent->key() && (keyEvent->modifiers() & Qt::ShiftModifier) && isFocusScin) {
            goToPrevElement();
        } else if (Qt::Key_Return == keyEvent->key() && isFocusScin) {
            goToNextElement();
        } else if(((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key()==Qt::Key_F) && isFocusScin) {
            _findPanel->show();
            _findLine->setFocus();
            _findLine->selectAll();
            return keyEvent->accept();
        } else {
            return BaseClass::keyPressEvent(keyEvent);
        }
    }

    void FindFrame::goToNextElement()
    {
        const QString &text = _findLine->text();
        if(!text.isEmpty())
        {
            bool re = false;
            bool wo = false;
            bool wrap = false;
            int index = 0;
            int line = 0;
            _scin->getCursorPosition(&line, &index);

            // Without the following line it is possible that searched
            // text will be selected, but window will not be correctly
            // scrolled (in case of very long lines). This line fixes it.
            _scin->setCursorPosition(line, 0);

            _scin->findFirst(text, re, _caseSensitive->checkState() == Qt::Checked, wo, wrap, true, line, index);
        }
    }

    FindFrame::~FindFrame()
    {
        delete _scin;
    }

    void FindFrame::goToPrevElement()
    {
        const QString &text = _findLine->text();
        if(!text.isEmpty())
        {
            bool re = false;
            bool wo = false;
            bool wrap = false;
            int index = 0;
            int line = 0;
            _scin->getCursorPosition(&line, &index);
            index -= _scin->selectedText().length();

            // Without the following line it is possible that searched
            // text will be selected, but window will not be correctly
            // scrolled (in case of very long lines). This line fixes it.
            _scin->setCursorPosition(line, 0);

            _scin->findFirst(text, re, _caseSensitive->checkState() == Qt::Checked, wo, wrap, false, line, index);
        }
    }
}
