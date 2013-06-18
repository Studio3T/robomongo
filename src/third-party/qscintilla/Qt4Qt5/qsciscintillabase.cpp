// This module implements the "official" low-level API.
//
// Copyright (c) 2012 Riverbank Computing Limited <info@riverbankcomputing.com>
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


#include "Qsci/qsciscintillabase.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qcolor.h>
#include <qscrollbar.h>
#include <qtextcodec.h>

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QList>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>

#include "ScintillaQt.h"


// The #defines in Scintilla.h and the enums in qsciscintillabase.h conflict
// (because we want to use the same names) so we have to undefine those we use
// in this file.
#undef  SCI_SETCARETPERIOD
#undef  SCK_DOWN
#undef  SCK_UP
#undef  SCK_LEFT
#undef  SCK_RIGHT
#undef  SCK_HOME
#undef  SCK_END
#undef  SCK_PRIOR
#undef  SCK_NEXT
#undef  SCK_DELETE
#undef  SCK_INSERT
#undef  SCK_ESCAPE
#undef  SCK_BACK
#undef  SCK_TAB
#undef  SCK_RETURN
#undef  SCK_ADD
#undef  SCK_SUBTRACT
#undef  SCK_DIVIDE
#undef  SCK_WIN
#undef  SCK_RWIN
#undef  SCK_MENU


// Remember if we have linked the lexers.
static bool lexersLinked = false;

// The list of instances.
static QList<QsciScintillaBase *> poolList;

// Mime support.
static const QLatin1String mimeTextPlain("text/plain");
static const QLatin1String mimeRectangularWin("MSDEVColumnSelect");
static const QLatin1String mimeRectangular("text/x-qscintilla-rectangular");
static const QLatin1String utiRectangularMac("com.scintilla.utf16-plain-text.rectangular");

// FIXME: QMacPasteboardMime isn't in Qt v5 yet.
#if QT_VERSION >= 0x040200 && defined(Q_OS_MAC) && QT_VERSION < 0x050000
#include <QMacPasteboardMime>

class RectangularPasteboardMime : public QMacPasteboardMime
{
public:
    RectangularPasteboardMime() : QMacPasteboardMime(MIME_ALL)
    {
    }

    bool canConvert(const QString &mime, QString flav)
    {
        return mime == mimeRectangular && flav == utiRectangularMac;
    }

    QList<QByteArray> convertFromMime(const QString &, QVariant data, QString)
    {
        QList<QByteArray> converted;

        converted.append(data.toByteArray());

        return converted;
    }

    QVariant convertToMime(const QString &, QList<QByteArray> data, QString)
    {
        QByteArray converted;

        foreach (QByteArray i, data)
        {
            converted += i;
        }

        return QVariant(converted);
    }

    QString convertorName()
    {
        return QString("QScintillaRectangular");
    }

    QString flavorFor(const QString &mime)
    {
        if (mime == mimeRectangular)
            return QString(utiRectangularMac);

        return QString();
    }

    QString mimeFor(QString flav)
    {
        if (flav == utiRectangularMac)
            return QString(mimeRectangular);

        return QString();
    }

    static void initialise()
    {
        if (!instance)
        {
            instance = new RectangularPasteboardMime();

            qRegisterDraggedTypes(QStringList(utiRectangularMac));
        }
    }

private:
    static RectangularPasteboardMime *instance;
};

RectangularPasteboardMime *RectangularPasteboardMime::instance;
#endif


// The ctor.
QsciScintillaBase::QsciScintillaBase(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)),
            SLOT(handleVSb(int)));

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
            SLOT(handleHSb(int)));

    setAcceptDrops(true);
    setFocusPolicy(Qt::WheelFocus);
    setAttribute(Qt::WA_KeyCompression);

    viewport()->setBackgroundRole(QPalette::Base);
    viewport()->setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);

    triple_click.setSingleShot(true);

// FIXME: QMacPasteboardMime isn't in Qt v5 yet.
#if QT_VERSION >= 0x040200 && defined(Q_OS_MAC) && QT_VERSION < 0x050000
    RectangularPasteboardMime::initialise();
#endif

    sci = new QsciScintillaQt(this);

    SendScintilla(SCI_SETCARETPERIOD, QApplication::cursorFlashTime() / 2);

    // Make sure the lexers are linked in.
    if (!lexersLinked)
    {
        Scintilla_LinkLexers();
        lexersLinked = true;
    }

    QClipboard *cb = QApplication::clipboard();

    if (cb->supportsSelection())
        connect(cb, SIGNAL(selectionChanged()), SLOT(handleSelection()));

    // Add it to the pool.
    poolList.append(this);
}


// The dtor.
QsciScintillaBase::~QsciScintillaBase()
{
    // Remove it from the pool.
    poolList.removeAt(poolList.indexOf(this));

    delete sci;
}


// Return an instance from the pool.
QsciScintillaBase *QsciScintillaBase::pool()
{
    return poolList.first();
}


// Tell Scintilla to update the scroll bars.  Scintilla should be doing this
// itself.
void QsciScintillaBase::setScrollBars()
{
    sci->SetScrollBars();
}


// Send a message to the real Scintilla widget using the low level Scintilla
// API.
long QsciScintillaBase::SendScintilla(unsigned int msg, unsigned long wParam,
        long lParam) const
{
    return sci->WndProc(msg, wParam, lParam);
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, unsigned long wParam,
        void *lParam) const
{
    return sci->WndProc(msg, wParam, reinterpret_cast<sptr_t>(lParam));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, unsigned long wParam,
        const char *lParam) const
{
    return sci->WndProc(msg, wParam, reinterpret_cast<sptr_t>(lParam));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg,
        const char *lParam) const
{
    return sci->WndProc(msg, static_cast<uptr_t>(0),
            reinterpret_cast<sptr_t>(lParam));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, const char *wParam,
        const char *lParam) const
{
    return sci->WndProc(msg, reinterpret_cast<uptr_t>(wParam),
            reinterpret_cast<sptr_t>(lParam));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, long wParam) const
{
    return sci->WndProc(msg, static_cast<uptr_t>(wParam),
            static_cast<sptr_t>(0));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, int wParam) const
{
    return sci->WndProc(msg, static_cast<uptr_t>(wParam),
            static_cast<sptr_t>(0));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, long cpMin, long cpMax,
        char *lpstrText) const
{
    QSCI_SCI_NAMESPACE(TextRange) tr;

    tr.chrg.cpMin = cpMin;
    tr.chrg.cpMax = cpMax;
    tr.lpstrText = lpstrText;

    return sci->WndProc(msg, static_cast<uptr_t>(0),
            reinterpret_cast<sptr_t>(&tr));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, unsigned long wParam,
        const QColor &col) const
{
    sptr_t lParam = (col.blue() << 16) | (col.green() << 8) | col.red();

    return sci->WndProc(msg, wParam, lParam);
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, const QColor &col) const
{
    uptr_t wParam = (col.blue() << 16) | (col.green() << 8) | col.red();

    return sci->WndProc(msg, wParam, static_cast<sptr_t>(0));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, unsigned long wParam,
        QPainter *hdc, const QRect &rc, long cpMin, long cpMax) const
{
    QSCI_SCI_NAMESPACE(RangeToFormat) rf;

    rf.hdc = rf.hdcTarget = reinterpret_cast<QSCI_SCI_NAMESPACE(SurfaceID)>(hdc);

    rf.rc.left = rc.left();
    rf.rc.top = rc.top();
    rf.rc.right = rc.right() + 1;
    rf.rc.bottom = rc.bottom() + 1;

    rf.chrg.cpMin = cpMin;
    rf.chrg.cpMax = cpMax;

    return sci->WndProc(msg, wParam, reinterpret_cast<sptr_t>(&rf));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, unsigned long wParam,
        const QPixmap &lParam) const
{
    return sci->WndProc(msg, wParam, reinterpret_cast<sptr_t>(&lParam));
}


// Overloaded message send.
long QsciScintillaBase::SendScintilla(unsigned int msg, unsigned long wParam,
        const QImage &lParam) const
{
    return sci->WndProc(msg, wParam, reinterpret_cast<sptr_t>(&lParam));
}


// Send a message to the real Scintilla widget using the low level Scintilla
// API that returns a pointer result.
void *QsciScintillaBase::SendScintillaPtrResult(unsigned int msg) const
{
    return reinterpret_cast<void *>(sci->WndProc(msg, static_cast<uptr_t>(0),
            static_cast<sptr_t>(0)));
}


// Handle the timer on behalf of the QsciScintillaQt instance.
void QsciScintillaBase::handleTimer()
{
    sci->Tick();
}


// Re-implemented to handle the context menu.
void QsciScintillaBase::contextMenuEvent(QContextMenuEvent *e)
{
    sci->ContextMenu(QSCI_SCI_NAMESPACE(Point)(e->globalX(), e->globalY()));
}


// Re-implemented to tell the widget it has the focus.
void QsciScintillaBase::focusInEvent(QFocusEvent *)
{
    sci->SetFocusState(true);
}


// Re-implemented to tell the widget it has lost the focus.
void QsciScintillaBase::focusOutEvent(QFocusEvent *)
{
    sci->SetFocusState(false);
}


// Re-implemented to make sure tabs are passed to the editor.
bool QsciScintillaBase::focusNextPrevChild(bool next)
{
    if (!sci->pdoc->IsReadOnly())
        return false;

    return QAbstractScrollArea::focusNextPrevChild(next);
}


// Handle the selection changing.
void QsciScintillaBase::handleSelection()
{
    if (!QApplication::clipboard()->ownsSelection())
        sci->UnclaimSelection();
}


// Handle key presses.
void QsciScintillaBase::keyPressEvent(QKeyEvent *e)
{
    unsigned key, modifiers = 0;

    if (e->modifiers() & Qt::ShiftModifier)
        modifiers |= SCMOD_SHIFT;

    if (e->modifiers() & Qt::ControlModifier)
        modifiers |= SCMOD_CTRL;

    if (e->modifiers() & Qt::AltModifier)
        modifiers |= SCMOD_ALT;

    if (e->modifiers() & Qt::MetaModifier)
        modifiers |= SCMOD_META;

    switch (e->key())
    {
    case Qt::Key_Down:
        key = SCK_DOWN;
        break;

    case Qt::Key_Up:
        key = SCK_UP;
        break;

    case Qt::Key_Left:
        key = SCK_LEFT;
        break;

    case Qt::Key_Right:
        key = SCK_RIGHT;
        break;

    case Qt::Key_Home:
        key = SCK_HOME;
        break;

    case Qt::Key_End:
        key = SCK_END;
        break;

    case Qt::Key_PageUp:
        key = SCK_PRIOR;
        break;

    case Qt::Key_PageDown:
        key = SCK_NEXT;
        break;

    case Qt::Key_Delete:
        key = SCK_DELETE;
        break;

    case Qt::Key_Insert:
        key = SCK_INSERT;
        break;

    case Qt::Key_Escape:
        key = SCK_ESCAPE;
        break;

    case Qt::Key_Backspace:
        key = SCK_BACK;
        break;

    case Qt::Key_Tab:
        key = SCK_TAB;
        break;

    case Qt::Key_Backtab:
        // Scintilla assumes a backtab is shift-tab.
        key = SCK_TAB;
        modifiers |= SCMOD_SHIFT;
        break;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        key = SCK_RETURN;
        break;

    case Qt::Key_Super_L:
        key = SCK_WIN;
        break;

    case Qt::Key_Super_R:
        key = SCK_RWIN;
        break;

    case Qt::Key_Menu:
        key = SCK_MENU;
        break;

    default:
        key = e->key();
    }

    if (key)
    {
        bool consumed = false;

        sci->KeyDownWithModifiers(key, modifiers, &consumed);

        if (consumed)
        {
            e->accept();
            return;
        }
    }

    QString text = e->text();

    if (!text.isEmpty() && text[0].isPrint())
    {
        QByteArray enc_text;

        if (sci->IsUnicodeMode())
        {
            enc_text = text.toUtf8();
        }
        else
        {
            static QTextCodec *codec = 0;

            if (!codec)
                codec = QTextCodec::codecForName("ISO 8859-1");

            enc_text = codec->fromUnicode(text);
        }

        sci->AddCharUTF(enc_text.data(), enc_text.length());
        e->accept();
    }
    else
    {
        QAbstractScrollArea::keyPressEvent(e);
    }
}


// Handle composed characters.  Note that this is the minumum needed to retain
// the QScintilla v1 functionality.
void QsciScintillaBase::inputMethodEvent(QInputMethodEvent *e)
{
    QByteArray utf8 = e->commitString().toUtf8();

    sci->AddCharUTF(utf8.data(), utf8.length());
    e->accept();
}


// Handle a mouse button double click.
void QsciScintillaBase::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
    {
        e->ignore();
        return;
    }

    setFocus();

    // Make sure Scintilla will interpret this as a double-click.
    unsigned clickTime = sci->lastClickTime + QSCI_SCI_NAMESPACE(Platform)::DoubleClickTime() - 1;

    bool shift = e->modifiers() & Qt::ShiftModifier;
    bool ctrl = e->modifiers() & Qt::ControlModifier;
    bool alt = e->modifiers() & Qt::AltModifier;

    sci->ButtonDown(QSCI_SCI_NAMESPACE(Point)(e->x(), e->y()), clickTime,
            shift, ctrl, alt);

    // Remember the current position and time in case it turns into a triple
    // click.
    triple_click_at = e->globalPos();
    triple_click.start(QApplication::doubleClickInterval());
}


// Handle a mouse move.
void QsciScintillaBase::mouseMoveEvent(QMouseEvent *e)
{
    sci->ButtonMove(QSCI_SCI_NAMESPACE(Point)(e->x(), e->y()));
}


// Handle a mouse button press.
void QsciScintillaBase::mousePressEvent(QMouseEvent *e)
{
    setFocus();

    QSCI_SCI_NAMESPACE(Point) pt(e->x(), e->y());

    if (e->button() == Qt::LeftButton)
    {
        unsigned clickTime;

        // It is a triple click if the timer is running and the mouse hasn't
        // moved too much.
        if (triple_click.isActive() && (e->globalPos() - triple_click_at).manhattanLength() < QApplication::startDragDistance())
            clickTime = sci->lastClickTime + QSCI_SCI_NAMESPACE(Platform)::DoubleClickTime() - 1;
        else
            clickTime = sci->lastClickTime + QSCI_SCI_NAMESPACE(Platform)::DoubleClickTime() + 1;

        triple_click.stop();

        // Scintilla uses the Alt modifier to initiate rectangular selection.
        // However the GTK port (under X11, not Windows) uses the Control
        // modifier (by default, although it is configurable).  It does this
        // because most X11 window managers hijack Alt-drag to move the window.
        // We do the same, except that (for the moment at least) we don't allow
        // the modifier to be configured.
        bool shift = e->modifiers() & Qt::ShiftModifier;
        bool ctrl = e->modifiers() & Qt::ControlModifier;
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
        bool alt = e->modifiers() & Qt::AltModifier;
#else
        bool alt = ctrl;
#endif

        sci->ButtonDown(pt, clickTime, shift, ctrl, alt);
    }
    else if (e->button() == Qt::MidButton)
    {
        QClipboard *cb = QApplication::clipboard();

        if (cb->supportsSelection())
        {
            int pos = sci->PositionFromLocation(pt);

            sci->sel.Clear();
            sci->SetSelection(pos, pos);

            sci->pasteFromClipboard(QClipboard::Selection);
        }
    }
}


// Handle a mouse button releases.
void QsciScintillaBase::mouseReleaseEvent(QMouseEvent *e)
{
    if (sci->HaveMouseCapture() && e->button() == Qt::LeftButton)
    {
        bool ctrl = e->modifiers() & Qt::ControlModifier;

        sci->ButtonUp(QSCI_SCI_NAMESPACE(Point)(e->x(), e->y()), 0, ctrl);
    }
}


// Handle paint events.
void QsciScintillaBase::paintEvent(QPaintEvent *e)
{
    sci->paintEvent(e);
}


// Handle resize events.
void QsciScintillaBase::resizeEvent(QResizeEvent *)
{
    sci->ChangeSize();
}


// Re-implemented to suppress the default behaviour as Scintilla works at a
// more fundamental level.
void QsciScintillaBase::scrollContentsBy(int, int)
{
}


// Handle the vertical scrollbar.
void QsciScintillaBase::handleVSb(int value)
{
    sci->ScrollTo(value);
}


// Handle the horizontal scrollbar.
void QsciScintillaBase::handleHSb(int value)
{
    sci->HorizontalScrollTo(value);
}


// Handle drag enters.
void QsciScintillaBase::dragEnterEvent(QDragEnterEvent *e)
{
    QsciScintillaBase::dragMoveEvent(e);
}


// Handle drag leaves.
void QsciScintillaBase::dragLeaveEvent(QDragLeaveEvent *)
{
    sci->SetDragPosition(QSCI_SCI_NAMESPACE(SelectionPosition)());
}


// Handle drag moves.
void QsciScintillaBase::dragMoveEvent(QDragMoveEvent *e)
{
    sci->SetDragPosition(
            sci->SPositionFromLocation(
                    QSCI_SCI_NAMESPACE(Point)(e->pos().x(), e->pos().y()),
                    false, false, sci->UserVirtualSpace()));

    acceptAction(e);
}


// Handle drops.
void QsciScintillaBase::dropEvent(QDropEvent *e)
{
    bool moving;
    int len;
    const char *s;
    bool rectangular;

    acceptAction(e);

    if (!e->isAccepted())
        return;

    moving = (e->dropAction() == Qt::MoveAction);

    QByteArray text = fromMimeData(e->mimeData(), rectangular);
    len = text.length();
    s = text.data();

    s = QSCI_SCI_NAMESPACE(Document)::TransformLineEnds(&len, s, len,
                sci->pdoc->eolMode);

    sci->DropAt(sci->posDrop, s, moving, rectangular);

    delete[] s;

    sci->Redraw();
}


void QsciScintillaBase::acceptAction(QDropEvent *e)
{
    if (sci->pdoc->IsReadOnly() || !canInsertFromMimeData(e->mimeData()))
        e->ignore();
    else
        e->acceptProposedAction();
}



// See if a MIME data object can be decoded.
bool QsciScintillaBase::canInsertFromMimeData(const QMimeData *source) const
{
    return source->hasFormat(mimeTextPlain);
}


// Create text from a MIME data object.
QByteArray QsciScintillaBase::fromMimeData(const QMimeData *source, bool &rectangular) const
{
    // See if it is rectangular.  We try all of the different formats that
    // Scintilla supports in case we are working across different platforms.
    if (source->hasFormat(mimeRectangularWin))
        rectangular = true;
    else if (source->hasFormat(mimeRectangular))
        rectangular = true;
    else
        rectangular = false;

    // Note that we don't support Scintilla's hack of adding a '\0' as Qt
    // strips it off under the covers when pasting from another process.
    QString utf8 = source->text();
    QByteArray text;

    if (sci->IsUnicodeMode())
        text = utf8.toUtf8();
    else
        text = utf8.toLatin1();

    return text;
}


// Create a MIME data object for some text.
QMimeData *QsciScintillaBase::toMimeData(const QByteArray &text, bool rectangular) const
{
    QMimeData *mime = new QMimeData;

    QString utf8;

    if (sci->IsUnicodeMode())
        utf8 = QString::fromUtf8(text.constData(), text.size());
    else
        utf8 = QString::fromLatin1(text.constData(), text.size());

    mime->setText(utf8);

    if (rectangular)
    {
        // Use the platform specific "standard" for specifying a rectangular
        // selection.
#if defined(Q_OS_WIN)
        mime->setData(mimeRectangularWin, QByteArray());
#else
        mime->setData(mimeRectangular, QByteArray());
#endif
    }

    return mime;
}

