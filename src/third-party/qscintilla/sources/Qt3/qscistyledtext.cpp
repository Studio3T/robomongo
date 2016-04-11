// This module implements the QsciStyledText class.
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


#include "Qsci/qscistyledtext.h"

#include "Qsci/qsciscintillabase.h"
#include "Qsci/qscistyle.h"


// A ctor.
QsciStyledText::QsciStyledText(const QString &text, int style)
    : styled_text(text), style_nr(style), explicit_style(0)
{
}


// A ctor.
QsciStyledText::QsciStyledText(const QString &text, const QsciStyle &style)
    : styled_text(text), style_nr(-1)
{
    explicit_style = new QsciStyle(style);
}


// Return the number of the style.
int QsciStyledText::style() const
{
    return explicit_style ? explicit_style->style() : style_nr;
}


// Apply any explicit style to an editor.
void QsciStyledText::apply(QsciScintillaBase *sci) const
{
    if (explicit_style)
        explicit_style->apply(sci);
}
