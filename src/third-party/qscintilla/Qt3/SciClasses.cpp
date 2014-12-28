// The implementation of various Qt version independent classes used by the
// rest of the port.
//
// Copyright (c) 2014 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of QScintilla.
// 
// This file may be used under the terms of the GNU General Public
// License versions 2.0 or 3.0 as published by the Free Software
// Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
// included in the packaging of this file.  Alternatively you may (at
// your option) use any later version of the GNU General Public
// License if such license has been publicly approved by Riverbank
// Computing Limited (or its successors, if any) and the KDE Free Qt
// Foundation. In addition, as a special exception, Riverbank gives you
// certain additional rights. These rights are described in the Riverbank
// GPL Exception version 1.1, which can be found in the file
// GPL_EXCEPTION.txt in this package.
// 
// If you are unsure which license is appropriate for your use, please
// contact the sales department at sales@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#include "SciNamespace.h"
#include "SciClasses.h"

#include <qevent.h>
#include <qpainter.h>

#include "ScintillaQt.h"
#include "ListBoxQt.h"


// Create a call tip.
QsciSciCallTip::QsciSciCallTip(QWidget *parent, QsciScintillaQt *sci_)
    : QWidget(parent, 0, Qt::WType_Popup|Qt::WStyle_Customize|Qt::WStyle_NoBorder),
      sci(sci_)
{
    // Ensure that the main window keeps the focus (and the caret flashing)
    // when this is displayed.
    setFocusProxy(parent);
}


// Destroy a call tip.
QsciSciCallTip::~QsciSciCallTip()
{
    // Ensure that the main window doesn't receive a focus out event when
    // this is destroyed.
    setFocusProxy(0);
}


// Paint a call tip.
void QsciSciCallTip::paintEvent(QPaintEvent *)
{
    QSCI_SCI_NAMESPACE(Surface) *surfaceWindow = QSCI_SCI_NAMESPACE(Surface)::Allocate(SC_TECHNOLOGY_DEFAULT);

    if (!surfaceWindow)
        return;

    QPainter p(this);

    surfaceWindow->Init(&p);
    surfaceWindow->SetUnicodeMode(sci->CodePage() == SC_CP_UTF8);
    sci->ct.PaintCT(surfaceWindow);

    delete surfaceWindow;
}


// Handle a mouse press in a call tip.
void QsciSciCallTip::mousePressEvent(QMouseEvent *e)
{
    QSCI_SCI_NAMESPACE(Point) pt;

    pt.x = e->x();
    pt.y = e->y();

    sci->ct.MouseClick(pt);
    sci->CallTipClick();

    update();
}



// Add an item and associated command to the popup and enable it if required.
void QsciSciPopup::addItem(const QString &label, int cmd, bool enabled,
        QsciScintillaQt *sci_)
{
    insertItem(label, this, SLOT(on_triggered(int)), 0, cmd);
    setItemEnabled(cmd, enabled);
    sci = sci_;
}


// Add a separator to the popup.
void QsciSciPopup::addSeparator()
{
    insertSeparator();
}


// A slot to handle a menu action being triggered.
void QsciSciPopup::on_triggered(int cmd)
{
    sci->Command(cmd);
}



QsciSciListBox::QsciSciListBox(QWidget *parent, QsciListBoxQt *lbx_)
    : QListBox(parent,0,Qt::WType_Popup|Qt::WStyle_Customize|Qt::WStyle_NoBorder|Qt::WStaticContents), lbx(lbx_)
{
    setFocusProxy(parent);

    setFrameShape(StyledPanel);
    setFrameShadow(Plain);

    connect(this,SIGNAL(doubleClicked(QListBoxItem *)),
        SLOT(handleSelection()));

    connect(this,SIGNAL(highlighted(QListBoxItem *)),
        SLOT(ensureCurrentVisible()));
}


int QsciSciListBox::find(const QString &prefix)
{
    return index(findItem(prefix, Qt::CaseSensitive|Qt::BeginsWith));
}



QsciSciListBox::~QsciSciListBox()
{
    // Ensure that the main widget doesn't get a focus out event when this is
    // destroyed.
    setFocusProxy(0);
}


void QsciSciListBox::handleSelection()
{
    if (lbx && lbx->cb_action)
        lbx->cb_action(lbx->cb_data);
}
