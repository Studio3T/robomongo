// This defines the interface to the QsciDocument class.
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


#ifndef QSCIDOCUMENT_H
#define QSCIDOCUMENT_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <Qsci/qsciglobal.h>


class QsciScintillaBase;
class QsciDocumentP;


//! \brief The QsciDocument class represents a document to be edited.
//!
//! It is an opaque class that can be attached to multiple instances of
//! QsciScintilla to create different simultaneous views of the same document.
//! QsciDocument uses implicit sharing so that copying class instances is a
//! cheap operation.
class QSCINTILLA_EXPORT QsciDocument
{
public:
    //! Create a new unattached document.
    QsciDocument();
    virtual ~QsciDocument();

    QsciDocument(const QsciDocument &);
    QsciDocument &operator=(const QsciDocument &);

private:
    friend class QsciScintilla;

    void attach(const QsciDocument &that);
    void detach();
    void display(QsciScintillaBase *qsb, const QsciDocument *from);
    void undisplay(QsciScintillaBase *qsb);

    bool isModified() const;
    void setModified(bool m);

    QsciDocumentP *pdoc;
};

#ifdef __APPLE__
}
#endif

#endif
