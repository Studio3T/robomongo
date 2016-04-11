#include "robomongo/gui/editors/FindFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QToolButton>
#include <Qsci/qsciscintilla.h>
#include <QMessageBox>
#include <QKeyEvent>

#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/KeyboardManager.h"
#include "robomongo/gui/widgets/workarea/ScriptWidget.h"

namespace Robomongo
{
    FindFrame::FindFrame(QWidget *parent) : 
        BaseClass(parent),
        _parent(parent),
        _scin(new RoboScintilla()),
        _findPanel(new QFrame(this)),
        _close(new QToolButton(this)),
        _findLine(new QLineEdit(this)),
        _next(new QPushButton("Next", this)),
        _prev(new QPushButton("Previous", this)),
        _caseSensitive(new QCheckBox("Match case", this)),
        _commentSign("// "),
        _commentSignLength(3)
    {
        _close->setIcon(QIcon(":/robomongo/icons/close_2_16x16.png"));
        _close->setToolButtonStyle(Qt::ToolButtonIconOnly);
        _close->setIconSize(QSize(16, 16));
        _close->hide(); // We do not need close button because ESC works

        _findLine->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);

        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(2, 0, 6, 0);
        layout->setSpacing(7);
        layout->addWidget(_findLine);
        layout->addWidget(_next);
        layout->addWidget(_prev);
        layout->addWidget(_caseSensitive);

        _findPanel->setFixedHeight(HeightFindPanel);
        _findPanel->setLayout(layout);

        QVBoxLayout *mainLayout = new QVBoxLayout();
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->addWidget(_scin, 1);
        mainLayout->addWidget(_findPanel, 0, Qt::AlignBottom);
        setLayout(mainLayout);

        _findPanel->hide();

        VERIFY(connect(_close, SIGNAL(clicked()), _findPanel, SLOT(hide())));
        VERIFY(connect(_next, SIGNAL(clicked()), this, SLOT(goToNextElement())));
        VERIFY(connect(_prev, SIGNAL(clicked()), this, SLOT(goToPrevElement())));
    }

    void FindFrame::wheelEvent(QWheelEvent *e)
    {
        return BaseClass::wheelEvent(e);
    }

    void FindFrame::keyPressEvent(QKeyEvent *keyEvent)
    {
        bool isFocusScin = _scin->isActiveWindow();
        bool isShowFind = _findPanel->isVisible();
        if (Qt::Key_Escape == keyEvent->key() && isFocusScin && isShowFind) {
            // Hide & Show of Scintilla widget solves problem of UI blinking
            _scin->hide();
            _findPanel->hide();
            _scin->setFocus();
            _scin->show();
            return keyEvent->accept();
        } else if (Qt::Key_Return == keyEvent->key() && (keyEvent->modifiers() & Qt::ShiftModifier) && isFocusScin && isShowFind) {
            goToPrevElement();
        } else if (Qt::Key_Return == keyEvent->key() && isFocusScin && isShowFind) {
            goToNextElement();
        } else if (((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() == Qt::Key_F) && isFocusScin) {
            _findPanel->show();
            _findLine->setFocus();
            _findLine->selectAll();
            return keyEvent->accept();
        } else if (KeyboardManager::isToggleCommentsShortcut(keyEvent)) {
            toggleComments();
            return keyEvent->accept();
        } else {
            return BaseClass::keyPressEvent(keyEvent);
        }
    }

    void FindFrame::goToNextElement()
    {
        findElement(true);
    }

    void FindFrame::goToPrevElement()
    {
        findElement(false);
    }

    void FindFrame::findElement(bool forward)
    {
        const QString &text = _findLine->text();
        if (!text.isEmpty()) {
            bool re = false;
            bool wo = false;
            bool looped = true;
            int index = 0;
            int line = 0;
            _scin->getCursorPosition(&line, &index);

            if (!forward)
               index -= _scin->selectedText().length();

            _scin->setCursorPosition(line, 0);
            bool isFounded = _scin->findFirst(text, re, _caseSensitive->checkState() == Qt::Checked, wo, looped, forward, line, index);

            if (isFounded) {
                _scin->ensureCursorVisible(); 
            }
            else {
                QMessageBox::warning(this, tr("Search"), tr("The specified text was not found."));
            }            
        }
    }
    
    void FindFrame::toggleComments()
    {
        int lineFrom, indexFrom, lineTo, indexTo;
        QString line;
        ScriptWidget *container;
        bool commentOut, is_textAndCursorNotificationsDisabled;
                
        _scin->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
        if (-1 == indexTo) {
            // There is no selection. Get cursor position
            _scin->getCursorPosition(&lineFrom, &indexFrom);
            lineTo = lineFrom;
        }
        // Define what action should be done for each selected line        
        line = _scin->text(lineFrom);
        if (line.startsWith(_commentSign)) {
            // Remove comment sign
            commentOut = false;
        } else {
            // Add comment sign
            commentOut = true;
        }
        
        // To prevent displaying of autocomplete menu
        container = dynamic_cast<ScriptWidget*>(_parent);
        if (NULL != container) {
            is_textAndCursorNotificationsDisabled = container->getDisableTextAndCursorNotifications();
            container->setDisableTextAndCursorNotifications(true);
        }

        for (int lineIndex = lineFrom; lineIndex <= lineTo; ++lineIndex) {
            setLineComment(lineIndex, commentOut);
        }
        
        /** 
         * Changing cursor position cancels the selection, so restoring original position of the cursor is done 
         * only if there was no selection at the beginning of the operation
         */
        if (-1 == indexTo) {
            // No selection, set original cursor position
            if (commentOut) {
                _scin->setCursorPosition(lineFrom, indexFrom + _commentSignLength);
            } else {
                _scin->setCursorPosition(lineFrom, indexFrom - _commentSignLength);
            }
        } else {
            // Restore original selection
            if (commentOut) {
                _scin->setSelection(lineFrom, indexFrom + _commentSignLength, lineTo, indexTo + _commentSignLength);
            } else {
                _scin->setSelection(lineFrom, indexFrom - _commentSignLength, lineTo, indexTo - _commentSignLength);
            }
        }
        
        if (NULL != container) {
            container->setDisableTextAndCursorNotifications(is_textAndCursorNotificationsDisabled);
        }
    }
    
    void FindFrame::setLineComment(const int lineIndex, const bool commentOut)
    {
        QString line;
        line = _scin->text(lineIndex);
        if (commentOut) {
            // Add comment sign
            _scin->insertAt(_commentSign, lineIndex, 0);
        } else if (line.startsWith(_commentSign)) {
            // Remove comment sign
            _scin->setSelection(lineIndex, 0, lineIndex, _commentSignLength);
            _scin->removeSelectedText();
        }
    }

    FindFrame::~FindFrame()
    {
        delete _scin;
    }
}
