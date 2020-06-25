// The implementation of the Qt specific subclass of ScintillaBase.
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


#include <string.h>

#include <qapplication.h>
#include <qevent.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <qstring.h>

#include <qbytearray.h>
#include <qdrag.h>
#include <qmimedata.h>

#include "Qsci/qsciscintillabase.h"
#include "ScintillaQt.h"
#include "SciClasses.h"


// We want to use the Scintilla notification names as Qt signal names.
#undef  SCEN_CHANGE
#undef  SCN_AUTOCCANCELLED
#undef  SCN_AUTOCCHARDELETED
#undef  SCN_AUTOCSELECTION
#undef  SCN_CALLTIPCLICK
#undef  SCN_CHARADDED
#undef  SCN_DOUBLECLICK
#undef  SCN_DWELLEND
#undef  SCN_DWELLSTART
#undef  SCN_FOCUSIN
#undef  SCN_FOCUSOUT
#undef  SCN_HOTSPOTCLICK
#undef  SCN_HOTSPOTDOUBLECLICK
#undef  SCN_HOTSPOTRELEASECLICK
#undef  SCN_INDICATORCLICK
#undef  SCN_INDICATORRELEASE
#undef  SCN_MACRORECORD
#undef  SCN_MARGINCLICK
#undef  SCN_MODIFIED
#undef  SCN_MODIFYATTEMPTRO
#undef  SCN_NEEDSHOWN
#undef  SCN_PAINTED
#undef  SCN_SAVEPOINTLEFT
#undef  SCN_SAVEPOINTREACHED
#undef  SCN_STYLENEEDED
#undef  SCN_UPDATEUI
#undef  SCN_USERLISTSELECTION
#undef  SCN_ZOOM

enum
{
    SCEN_CHANGE = 768,
    SCN_AUTOCCANCELLED = 2025,
    SCN_AUTOCCHARDELETED = 2026,
    SCN_AUTOCSELECTION = 2022,
    SCN_CALLTIPCLICK = 2021,
    SCN_CHARADDED = 2001,
    SCN_DOUBLECLICK = 2006,
    SCN_DWELLEND = 2017,
    SCN_DWELLSTART = 2016,
    SCN_FOCUSIN = 2028,
    SCN_FOCUSOUT = 2029,
    SCN_HOTSPOTCLICK = 2019,
    SCN_HOTSPOTDOUBLECLICK = 2020,
    SCN_HOTSPOTRELEASECLICK = 2027,
    SCN_INDICATORCLICK = 2023,
    SCN_INDICATORRELEASE = 2024,
    SCN_MACRORECORD = 2009,
    SCN_MARGINCLICK = 2010,
    SCN_MODIFIED = 2008,
    SCN_MODIFYATTEMPTRO = 2004,
    SCN_NEEDSHOWN = 2011,
    SCN_PAINTED = 2013,
    SCN_SAVEPOINTLEFT = 2003,
    SCN_SAVEPOINTREACHED = 2002,
    SCN_STYLENEEDED = 2000,
    SCN_UPDATEUI = 2007,
    SCN_USERLISTSELECTION = 2014,
    SCN_ZOOM = 2018
};


// The ctor.
QsciScintillaQt::QsciScintillaQt(QsciScintillaBase *qsb_)
    : capturedMouse(false), qsb(qsb_)
{
    wMain = qsb->viewport();

    // We aren't a QObject so we use the API class to do QObject related things
    // for us.
    qsb->connect(&qtimer, SIGNAL(timeout()), SLOT(handleTimer()));
    
    Initialise();
}


// The dtor.
QsciScintillaQt::~QsciScintillaQt()
{ 
    Finalise();
}


// Initialise the instance.
void QsciScintillaQt::Initialise()
{
    SetTicking(true);
}


// Tidy up the instance.
void QsciScintillaQt::Finalise()
{
    SetTicking(false);
    ScintillaBase::Finalise();
}


// Start a drag.
void QsciScintillaQt::StartDrag()
{
    inDragDrop = ddDragging;

    QDrag *qdrag = new QDrag(qsb);
    qdrag->setMimeData(mimeSelection(drag));

# if QT_VERSION >= 0x040300
    Qt::DropAction action = qdrag->exec(Qt::MoveAction | Qt::CopyAction, Qt::MoveAction);
# else
    Qt::DropAction action = qdrag->start(Qt::MoveAction);
# endif

    // Remove the dragged text if it was a move to another widget or
    // application.
    if (action == Qt::MoveAction && qdrag->target() != qsb->viewport())
        ClearSelection();

    SetDragPosition(QSCI_SCI_NAMESPACE(SelectionPosition)());
    inDragDrop = ddNone;
}


// Re-implement to trap certain messages.
sptr_t QsciScintillaQt::WndProc(unsigned int iMessage, uptr_t wParam,
        sptr_t lParam)
{
    switch (iMessage)
    {
    case SCI_GETDIRECTFUNCTION:
        return reinterpret_cast<sptr_t>(DirectFunction);
    
    case SCI_GETDIRECTPOINTER:
        return reinterpret_cast<sptr_t>(this);
    }

    return ScintillaBase::WndProc(iMessage, wParam, lParam);
}


// Windows nonsense.
sptr_t QsciScintillaQt::DefWndProc(unsigned int, uptr_t, sptr_t)
{
    return 0;
}


// Manage the timer.
void QsciScintillaQt::SetTicking(bool on)
{
    if (timer.ticking != on)
    {
        timer.ticking = on;

        if (timer.ticking)
            qtimer.start(timer.tickSize);
        else
            qtimer.stop();
    }

    timer.ticksToWait = caret.period;
}


// Grab or release the mouse (and keyboard).
void QsciScintillaQt::SetMouseCapture(bool on)
{
    if (mouseDownCaptures)
        if (on)
            qsb->viewport()->grabMouse();
        else
            qsb->viewport()->releaseMouse();

    capturedMouse = on;
}


// Return true if the mouse/keyboard are currently grabbed.
bool QsciScintillaQt::HaveMouseCapture()
{
    return capturedMouse;
}


// Set the position of the vertical scrollbar.
void QsciScintillaQt::SetVerticalScrollPos()
{
    qsb->verticalScrollBar()->setValue(topLine);
}


// Set the position of the horizontal scrollbar.
void QsciScintillaQt::SetHorizontalScrollPos()
{
    qsb->horizontalScrollBar()->setValue(xOffset);
}


// Set the extent of the vertical and horizontal scrollbars and return true if
// the view needs re-drawing.
bool QsciScintillaQt::ModifyScrollBars(int nMax,int nPage)
{
    qsb->verticalScrollBar()->setMinimum(0);
    qsb->verticalScrollBar()->setMaximum(nMax - nPage + 1);
    qsb->verticalScrollBar()->setPageStep(nPage);
    qsb->verticalScrollBar()->setSingleStep(1);

    // QAbstractScrollArea ignores Qt::ScrollBarAsNeeded and shows the
    // horizontal scrollbar if a non-zero maximum is set.  That isn't the
    // behavior we want, so set the maximum to zero unless scrollWidth exceeds
    // the viewport.
    const int widthBeyondViewport = qMax(0,
            scrollWidth - qsb->viewport()->width());

    qsb->horizontalScrollBar()->setMinimum(0);
    qsb->horizontalScrollBar()->setMaximum(qMax(0, widthBeyondViewport - 1));
    qsb->horizontalScrollBar()->setPageStep(widthBeyondViewport / 10);

    return true;
}


// Called after SCI_SETWRAPMODE and SCI_SETHSCROLLBAR.
void QsciScintillaQt::ReconfigureScrollBars()
{
    // Hide or show the scrollbars if needed.
    bool hsb = (horizontalScrollBarVisible && !Wrapping());

    qsb->setHorizontalScrollBarPolicy(hsb ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    qsb->setVerticalScrollBarPolicy(verticalScrollBarVisible ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
}


// Notify interested parties of any change in the document.
void QsciScintillaQt::NotifyChange()
{
    emit qsb->SCEN_CHANGE();
}


// Notify interested parties of various events.  This is the main mapping
// between Scintilla notifications and Qt signals.
void QsciScintillaQt::NotifyParent(QSCI_SCI_NAMESPACE(SCNotification) scn)
{
    switch (scn.nmhdr.code)
    {
    case SCN_CALLTIPCLICK:
        emit qsb->SCN_CALLTIPCLICK(scn.position);
        break;

    case SCN_AUTOCCANCELLED:
        emit qsb->SCN_AUTOCCANCELLED();
        break;

    case SCN_AUTOCCHARDELETED:
        emit qsb->SCN_AUTOCCHARDELETED();
        break;

    case SCN_AUTOCSELECTION:
        emit qsb->SCN_AUTOCSELECTION(scn.text, scn.lParam);
        break;

    case SCN_CHARADDED:
        emit qsb->SCN_CHARADDED(scn.ch);
        break;

    case SCN_DOUBLECLICK:
        emit qsb->SCN_DOUBLECLICK(scn.position, scn.line, scn.modifiers);
        break;

    case SCN_DWELLEND:
        emit qsb->SCN_DWELLEND(scn.position, scn.x, scn.y);
        break;

    case SCN_DWELLSTART:
        emit qsb->SCN_DWELLSTART(scn.position, scn.x, scn.y);
        break;

    case SCN_FOCUSIN:
        emit qsb->SCN_FOCUSIN();
        break;

    case SCN_FOCUSOUT:
        emit qsb->SCN_FOCUSOUT();
        break;

    case SCN_HOTSPOTCLICK:
        emit qsb->SCN_HOTSPOTCLICK(scn.position, scn.modifiers);
        break;

    case SCN_HOTSPOTDOUBLECLICK:
        emit qsb->SCN_HOTSPOTDOUBLECLICK(scn.position, scn.modifiers);
        break;

    case SCN_HOTSPOTRELEASECLICK:
        emit qsb->SCN_HOTSPOTRELEASECLICK(scn.position, scn.modifiers);
        break;

    case SCN_INDICATORCLICK:
        emit qsb->SCN_INDICATORCLICK(scn.position, scn.modifiers);
        break;

    case SCN_INDICATORRELEASE:
        emit qsb->SCN_INDICATORRELEASE(scn.position, scn.modifiers);
        break;

    case SCN_MACRORECORD:
        emit qsb->SCN_MACRORECORD(scn.message, scn.wParam,
                reinterpret_cast<void *>(scn.lParam));
        break;

    case SCN_MARGINCLICK:
        emit qsb->SCN_MARGINCLICK(scn.position, scn.modifiers, scn.margin);
        break;

    case SCN_MODIFIED:
        {
            char *text;

            // Give some protection to the Python bindings.
            if (scn.text && (scn.modificationType & (SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT) != 0))
            {
                text = new char[scn.length + 1];
                memcpy(text, scn.text, scn.length);
                text[scn.length] = '\0';
            }
            else
            {
                text = 0;
            }

            emit qsb->SCN_MODIFIED(scn.position, scn.modificationType, text,
                    scn.length, scn.linesAdded, scn.line, scn.foldLevelNow,
                    scn.foldLevelPrev, scn.token, scn.annotationLinesAdded);

            if (text)
                delete[] text;

            break;
        }

    case SCN_MODIFYATTEMPTRO:
        emit qsb->SCN_MODIFYATTEMPTRO();
        break;

    case SCN_NEEDSHOWN:
        emit qsb->SCN_NEEDSHOWN(scn.position, scn.length);
        break;

    case SCN_PAINTED:
        emit qsb->SCN_PAINTED();
        break;

    case SCN_SAVEPOINTLEFT:
        emit qsb->SCN_SAVEPOINTLEFT();
        break;

    case SCN_SAVEPOINTREACHED:
        emit qsb->SCN_SAVEPOINTREACHED();
        break;

    case SCN_STYLENEEDED:
        emit qsb->SCN_STYLENEEDED(scn.position);
        break;

    case SCN_UPDATEUI:
        emit qsb->SCN_UPDATEUI(scn.updated);
        break;

    case SCN_USERLISTSELECTION:
        emit qsb->SCN_USERLISTSELECTION(scn.text, scn.wParam);
        break;

    case SCN_ZOOM:
        emit qsb->SCN_ZOOM();
        break;

    default:
        qWarning("Unknown notification: %u", scn.nmhdr.code);
    }
}



// Convert a selection to mime data.
QMimeData *QsciScintillaQt::mimeSelection(
        const QSCI_SCI_NAMESPACE(SelectionText) &text) const
{
    return qsb->toMimeData(QByteArray(text.Data()), text.rectangular);
}



// Copy the selected text to the clipboard.
void QsciScintillaQt::CopyToClipboard(
        const QSCI_SCI_NAMESPACE(SelectionText) &selectedText)
{
    QApplication::clipboard()->setMimeData(mimeSelection(selectedText));
}


// Implement copy.
void QsciScintillaQt::Copy()
{
    if (!sel.Empty())
    {
        QSCI_SCI_NAMESPACE(SelectionText) text;

        CopySelectionRange(&text);
        CopyToClipboard(text);
    }
}


// Implement pasting text.
void QsciScintillaQt::Paste()
{
    pasteFromClipboard(QClipboard::Clipboard);
}


// Paste text from either the clipboard or selection.
void QsciScintillaQt::pasteFromClipboard(QClipboard::Mode mode)
{
    int len;
    const char *s;
    bool rectangular;

    const QMimeData *source = QApplication::clipboard()->mimeData(mode);

    if (!source || !qsb->canInsertFromMimeData(source))
        return;

    QByteArray text = qsb->fromMimeData(source, rectangular);
    len = text.length();
    s = text.data();

    std::string dest = QSCI_SCI_NAMESPACE(Document)::TransformLineEnds(s, len,
            pdoc->eolMode);

    QSCI_SCI_NAMESPACE(SelectionText) selText;
    selText.Copy(dest, (IsUnicodeMode() ? SC_CP_UTF8 : 0),
            vs.styles[STYLE_DEFAULT].characterSet, rectangular, false);

    QSCI_SCI_NAMESPACE(UndoGroup) ug(pdoc);

    ClearSelection();

    QSCI_SCI_NAMESPACE(SelectionPosition) start = sel.IsRectangular()
            ? sel.Rectangular().Start() : sel.Range(sel.Main()).Start();

    if (selText.rectangular)
        PasteRectangular(start, selText.Data(), selText.Length());
    else
        InsertPaste(start, selText.Data(), selText.Length());

    NotifyChange();
    Redraw();
}


// Create a call tip window.
void QsciScintillaQt::CreateCallTipWindow(QSCI_SCI_NAMESPACE(PRectangle) rc)
{
    if (!ct.wCallTip.Created())
        ct.wCallTip = ct.wDraw = new QsciSciCallTip(qsb, this);

    QsciSciCallTip *w = reinterpret_cast<QsciSciCallTip *>(ct.wCallTip.GetID());

    w->resize(rc.right - rc.left, rc.bottom - rc.top);
    ct.wCallTip.Show();
}


// Add an item to the right button menu.
void QsciScintillaQt::AddToPopUp(const char *label, int cmd, bool enabled)
{
    QsciSciPopup *pm = static_cast<QsciSciPopup *>(popup.GetID());

    if (*label)
        pm->addItem(qApp->translate("ContextMenu", label), cmd, enabled, this);
    else
        pm->addSeparator();
}


// Claim the selection.
void QsciScintillaQt::ClaimSelection()
{
    bool isSel = !sel.Empty();

    if (isSel)
    {
        QClipboard *cb = QApplication::clipboard();

        // If we support X11 style selection then make it available now.
        if (cb->supportsSelection())
        {
            QSCI_SCI_NAMESPACE(SelectionText) text;

            CopySelectionRange(&text);

            if (text.Data())
                cb->setMimeData(mimeSelection(text), QClipboard::Selection);
        }

        primarySelection = true;
    }
    else
        primarySelection = false;

    emit qsb->QSCN_SELCHANGED(isSel);
}


// Unclaim the selection.
void QsciScintillaQt::UnclaimSelection()
{
    if (primarySelection)
    {
        primarySelection = false;
        qsb->viewport()->update();
    }
}


// Implemented to provide compatibility with the Windows version.
sptr_t QsciScintillaQt::DirectFunction(QsciScintillaQt *sciThis, unsigned int iMessage,
        uptr_t wParam, sptr_t lParam)
{
    return sciThis->WndProc(iMessage,wParam,lParam);
}


// Draw the contents of the widget.
void QsciScintillaQt::paintEvent(QPaintEvent *e)
{
    QSCI_SCI_NAMESPACE(Surface) *sw = QSCI_SCI_NAMESPACE(Surface)::Allocate(SC_TECHNOLOGY_DEFAULT);

    if (!sw)
        return;

    paintState = painting;

    const QRect &qr = e->rect();

    rcPaint.left = qr.left();
    rcPaint.top = qr.top();
    rcPaint.right = qr.right() + 1;
    rcPaint.bottom = qr.bottom() + 1;

    QSCI_SCI_NAMESPACE(PRectangle) rcText = GetTextRectangle();
    paintingAllText = rcPaint.Contains(rcText);

    QPainter painter(qsb->viewport());

    sw->Init(&painter);
    sw->SetUnicodeMode(CodePage() == SC_CP_UTF8);
    Paint(sw, rcPaint);

    delete sw;

    // If the painting area was insufficient to cover the new style or brace
    // highlight positions then repaint the whole thing.
    if (paintState == paintAbandoned)
        qsb->viewport()->update();

    paintState = notPainting;
}
