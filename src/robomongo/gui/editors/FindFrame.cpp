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
    FindFrame::FindFrame(QWidget *parent):
        base_class(parent),scin_(new RoboScintilla(this)),
        find_panel_(new QFrame(this)),close_(new QToolButton(this)),
        find_line_(new QTextEdit(this)),next_(new QPushButton("next",this)),prev_(new QPushButton("prev",this)),case_sens_(new QCheckBox("Match case",this))
    {
        close_->setIcon(QIcon(":/robomongo/icons/close_2_16x16.png"));
        close_->setToolButtonStyle(Qt::ToolButtonIconOnly);
        close_->setIconSize(QSize(16,16));
        find_line_->setAlignment(Qt::AlignLeft|Qt::AlignAbsolute);
        QVBoxLayout *main_layout = new QVBoxLayout();
        main_layout->addWidget(scin_);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(0);
        find_panel_->setMaximumHeight(height_find_panel);
        find_panel_->setMinimumHeight(height_find_panel);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(close_);
        layout->addWidget(find_line_);
        layout->addWidget(next_);
        layout->addWidget(prev_);
        layout->addWidget(case_sens_);

        find_panel_->setLayout(layout);
        main_layout->addWidget(find_panel_);
        setLayout(main_layout);
        find_panel_->hide();

        connect(close_, SIGNAL(clicked()), find_panel_, SLOT(hide()));
        connect(next_, SIGNAL(clicked()), this, SLOT(GoToNextElement()));
        connect(prev_, SIGNAL(clicked()), this, SLOT(GoToPrevElement()));
    }
    void FindFrame::wheelEvent(QWheelEvent *e)
    {
        return base_class::wheelEvent(e);
    }
    void FindFrame::keyPressEvent(QKeyEvent *keyEvent)
    {
        bool is_focus_scin = scin_->isActiveWindow();
        if(Qt::Key_Escape == keyEvent->key()&&is_focus_scin)
        {
            find_panel_->hide();
            scin_->setFocus();
            return keyEvent->accept();
        }
        else if(((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key()==Qt::Key_F)&&is_focus_scin)
        {
            find_panel_->show();
            find_line_->setFocus();
            return keyEvent->accept();
        }
        else
        {
            return base_class::keyPressEvent(keyEvent);
        }
    }
    void FindFrame::GoToNextElement()
    {
        const QString &text = find_line_->toPlainText();
        if(!text.isEmpty())
        {
            bool re=false;
            bool wo=false;
            bool wrap=false;
            int index =0;
            int line =0;
            scin_->getCursorPosition(&line,&index);

            scin_->findFirst(text,re,case_sens_->checkState()==Qt::Checked,wo,wrap,true,line,index);
        }
    }
    void FindFrame::GoToPrevElement()
    {
        const QString &text = find_line_->toPlainText();
        if(!text.isEmpty())
        {
            bool re=false;
            bool wo=false;
            bool wrap=false;
            int index =0;
            int line =0;
            scin_->getCursorPosition(&line,&index);
            index -= scin_->selectedText().length();
            scin_->findFirst(text,re,case_sens_->checkState()==Qt::Checked,wo,wrap,false,line,index);
        }
    }
}
