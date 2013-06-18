// This module implements the QsciLexerMakefile class.
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


#include "Qsci/qscilexermakefile.h"

#include <qcolor.h>
#include <qfont.h>


// The ctor.
QsciLexerMakefile::QsciLexerMakefile(QObject *parent)
    : QsciLexer(parent)
{
}


// The dtor.
QsciLexerMakefile::~QsciLexerMakefile()
{
}


// Returns the language name.
const char *QsciLexerMakefile::language() const
{
    return "Makefile";
}


// Returns the lexer name.
const char *QsciLexerMakefile::lexer() const
{
    return "makefile";
}


// Return the string of characters that comprise a word.
const char *QsciLexerMakefile::wordCharacters() const
{
    return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-";
}


// Returns the foreground colour of the text for a style.
QColor QsciLexerMakefile::defaultColor(int style) const
{
    switch (style)
    {
    case Default:
    case Operator:
        return QColor(0x00,0x00,0x00);

    case Comment:
        return QColor(0x00,0x7f,0x00);

    case Preprocessor:
        return QColor(0x7f,0x7f,0x00);

    case Variable:
        return QColor(0x00,0x00,0x80);

    case Target:
        return QColor(0xa0,0x00,0x00);

    case Error:
        return QColor(0xff,0xff,0x00);
    }

    return QsciLexer::defaultColor(style);
}


// Returns the end-of-line fill for a style.
bool QsciLexerMakefile::defaultEolFill(int style) const
{
    if (style == Error)
        return true;

    return QsciLexer::defaultEolFill(style);
}


// Returns the font of the text for a style.
QFont QsciLexerMakefile::defaultFont(int style) const
{
    QFont f;

    if (style == Comment)
#if defined(Q_OS_WIN)
        f = QFont("Comic Sans MS",9);
#elif defined(Q_OS_MAC)
        f = QFont("Comic Sans MS", 12);
#else
        f = QFont("Bitstream Vera Serif",9);
#endif
    else
        f = QsciLexer::defaultFont(style);

    return f;
}


// Returns the user name of a style.
QString QsciLexerMakefile::description(int style) const
{
    switch (style)
    {
    case Default:
        return tr("Default");

    case Comment:
        return tr("Comment");

    case Preprocessor:
        return tr("Preprocessor");

    case Variable:
        return tr("Variable");

    case Operator:
        return tr("Operator");

    case Target:
        return tr("Target");

    case Error:
        return tr("Error");
    }

    return QString();
}


// Returns the background colour of the text for a style.
QColor QsciLexerMakefile::defaultPaper(int style) const
{
    if (style == Error)
        return QColor(0xff,0x00,0x00);

    return QsciLexer::defaultPaper(style);
}
