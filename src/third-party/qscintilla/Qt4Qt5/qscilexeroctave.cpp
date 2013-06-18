// This module implements the QsciLexerOctave class.
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


#include "Qsci/qscilexeroctave.h"

#include <qcolor.h>
#include <qfont.h>


// The ctor.
QsciLexerOctave::QsciLexerOctave(QObject *parent)
    : QsciLexerMatlab(parent)
{
}


// The dtor.
QsciLexerOctave::~QsciLexerOctave()
{
}


// Returns the language name.
const char *QsciLexerOctave::language() const
{
    return "Octave";
}


// Returns the lexer name.
const char *QsciLexerOctave::lexer() const
{
    return "octave";
}


// Returns the set of keywords.
const char *QsciLexerOctave::keywords(int set) const
{
    if (set == 1)
        return
            "break case catch continue do else elseif end end_unwind_protect "
            "endfor endfunction endif endswitch endwhile for function "
            "global if otherwise persistent return switch try until "
            "unwind_protect unwind_protect_cleanup while";

    return 0;
}
