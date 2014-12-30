// This module implements the QsciDocument class.
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


#include "Qsci/qscidocument.h"
#include "Qsci/qsciscintillabase.h"


// This internal class encapsulates the underlying document and is shared by
// QsciDocument instances.
class QsciDocumentP
{
public:
    QsciDocumentP() : doc(0), nr_displays(0), nr_attaches(1), modified(false) {}

    void *doc;              // The Scintilla document.
    int nr_displays;        // The number of displays.
    int nr_attaches;        // The number of attaches.
    bool modified;          // Set if not at a save point.
};


// The ctor.
QsciDocument::QsciDocument()
{
    pdoc = new QsciDocumentP();
}


// The dtor.
QsciDocument::~QsciDocument()
{
    detach();
}


// The copy ctor.
QsciDocument::QsciDocument(const QsciDocument &that)
{
    attach(that);
}


// The assignment operator.
QsciDocument &QsciDocument::operator=(const QsciDocument &that)
{
    if (pdoc != that.pdoc)
    {
        detach();
        attach(that);
    }

    return *this;
}


// Attach an existing document to this one.
void QsciDocument::attach(const QsciDocument &that)
{
    ++that.pdoc->nr_attaches;
    pdoc = that.pdoc;
}


// Detach the underlying document.
void QsciDocument::detach()
{
    if (!pdoc)
        return;

    if (--pdoc->nr_attaches == 0)
    {
        if (pdoc->doc && pdoc->nr_displays == 0)
        {
            QsciScintillaBase *qsb = QsciScintillaBase::pool();

            // Release the explicit reference to the document.  If the pool is
            // empty then we just accept the memory leak.
            if (qsb)
                qsb->SendScintilla(QsciScintillaBase::SCI_RELEASEDOCUMENT, 0,
                        pdoc->doc);
        }

        delete pdoc;
    }

    pdoc = 0;
}


// Undisplay and detach the underlying document.
void QsciDocument::undisplay(QsciScintillaBase *qsb)
{
    if (--pdoc->nr_attaches == 0)
        delete pdoc;
    else if (--pdoc->nr_displays == 0)
    {
        // Create an explicit reference to the document to keep it alive.
        qsb->SendScintilla(QsciScintillaBase::SCI_ADDREFDOCUMENT, 0, pdoc->doc);
    }

    pdoc = 0;
}


// Display the underlying document.
void QsciDocument::display(QsciScintillaBase *qsb, const QsciDocument *from)
{
    void *ndoc = (from ? from->pdoc->doc : 0);

    // SCI_SETDOCPOINTER appears to reset the EOL mode so save and restore it.
    int eol_mode = qsb->SendScintilla(QsciScintillaBase::SCI_GETEOLMODE);

    qsb->SendScintilla(QsciScintillaBase::SCI_SETDOCPOINTER, 0, ndoc);
    ndoc = qsb->SendScintillaPtrResult(QsciScintillaBase::SCI_GETDOCPOINTER);

    qsb->SendScintilla(QsciScintillaBase::SCI_SETEOLMODE, eol_mode);

    pdoc->doc = ndoc;
    ++pdoc->nr_displays;
}


// Return the modified state of the document.
bool QsciDocument::isModified() const
{
    return pdoc->modified;
}


// Set the modified state of the document.
void QsciDocument::setModified(bool m)
{
    pdoc->modified = m;
}
