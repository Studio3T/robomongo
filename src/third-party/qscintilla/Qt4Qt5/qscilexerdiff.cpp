// This module implements the QsciLexerDiff class.
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


#include "Qsci/qscilexerdiff.h"

#include <qcolor.h>
#include <qfont.h>
#include <qsettings.h>


// The ctor.
QsciLexerDiff::QsciLexerDiff(QObject *parent)
    : QsciLexer(parent)
{
}


// The dtor.
QsciLexerDiff::~QsciLexerDiff()
{
}


// Returns the language name.
const char *QsciLexerDiff::language() const
{
    return "Diff";
}


// Returns the lexer name.
const char *QsciLexerDiff::lexer() const
{
    return "diff";
}


// Return the string of characters that comprise a word.
const char *QsciLexerDiff::wordCharacters() const
{
    return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-";
}


// Returns the foreground colour of the text for a style.
QColor QsciLexerDiff::defaultColor(int style) const
{
    switch (style)
    {
    case Default:
        return QColor(0x00,0x00,0x00);

    case Comment:
        return QColor(0x00,0x7f,0x00);

    case Command:
        return QColor(0x7f,0x7f,0x00);

    case Header:
        return QColor(0x7f,0x00,0x00);

    case Position:
        return QColor(0x7f,0x00,0x7f);

    case LineRemoved:
        return QColor(0x00,0x7f,0x7f);

    case LineAdded:
        return QColor(0x00,0x00,0x7f);

    case LineChanged:
        return QColor(0x7f,0x7f,0x7f);
    }

    return QsciLexer::defaultColor(style);
}


// Returns the user name of a style.
QString QsciLexerDiff::description(int style) const
{
    switch (style)
    {
    case Default:
        return tr("Default");

    case Comment:
        return tr("Comment");

    case Command:
        return tr("Command");

    case Header:
        return tr("Header");

    case Position:
        return tr("Position");

    case LineRemoved:
        return tr("Removed line");

    case LineAdded:
        return tr("Added line");

    case LineChanged:
        return tr("Changed line");
    }

    return QString();
}
