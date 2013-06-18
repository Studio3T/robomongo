// This module implements the QsciLexerCustom class.
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


#include "Qsci/qscilexercustom.h"

#include "Qsci/qsciscintilla.h"
#include "Qsci/qsciscintillabase.h"
#include "Qsci/qscistyle.h"


// The ctor.
QsciLexerCustom::QsciLexerCustom(QObject *parent)
    : QsciLexer(parent)
{
}


// The dtor.
QsciLexerCustom::~QsciLexerCustom()
{
}


// Start styling.
void QsciLexerCustom::startStyling(int start, int styleBits)
{
    if (!editor())
        return;

    if (styleBits == 0)
        styleBits = (1 << styleBitsNeeded()) - 1;

    editor()->SendScintilla(QsciScintillaBase::SCI_STARTSTYLING, start,
            styleBits);
}


// Set the style for a number of characters.
void QsciLexerCustom::setStyling(int length, int style)
{
    if (!editor())
        return;

    editor()->SendScintilla(QsciScintillaBase::SCI_SETSTYLING, length, style);
}


// Set the style for a number of characters.
void QsciLexerCustom::setStyling(int length, const QsciStyle &style)
{
    setStyling(length, style.style());
}


// Set the attached editor.
void QsciLexerCustom::setEditor(QsciScintilla *new_editor)
{
    if (editor())
        disconnect(editor(), SIGNAL(SCN_STYLENEEDED(int)), this,
                SLOT(handleStyleNeeded(int)));

    QsciLexer::setEditor(new_editor);

    if (editor())
        connect(editor(), SIGNAL(SCN_STYLENEEDED(int)), this,
                SLOT(handleStyleNeeded(int)));
}


// Return the number of style bits needed by the lexer.
int QsciLexerCustom::styleBitsNeeded() const
{
    return 5;
}


// Handle a request to style some text.
void QsciLexerCustom::handleStyleNeeded(int pos)
{
    int start = editor()->SendScintilla(QsciScintillaBase::SCI_GETENDSTYLED);
    int line = editor()->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION,
            start);
    start = editor()->SendScintilla(QsciScintillaBase::SCI_POSITIONFROMLINE,
            line);

    if (start != pos)
        styleText(start, pos);
}
