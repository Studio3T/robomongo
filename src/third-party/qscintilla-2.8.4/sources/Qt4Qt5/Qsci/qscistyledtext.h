// This module defines interface to the QsciStyledText class.
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


#ifndef QSCISTYLEDTEXT_H
#define QSCISTYLEDTEXT_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qstring.h>

#include <Qsci/qsciglobal.h>


class QsciScintillaBase;
class QsciStyle;


//! \brief The QsciStyledText class is a container for a piece of text and the
//! style used to display the text.
class QSCINTILLA_EXPORT QsciStyledText
{
public:
    //! Constructs a QsciStyledText instance for text \a text and style number
    //! \a style.
    QsciStyledText(const QString &text, int style);

    //! Constructs a QsciStyledText instance for text \a text and style \a
    //! style.
    QsciStyledText(const QString &text, const QsciStyle &style);


    //! \internal Apply the style to a particular editor.
    void apply(QsciScintillaBase *sci) const;

    //! Returns a reference to the text.
    const QString &text() const {return styled_text;}

    //! Returns the number of the style.
    int style() const;

private:
    QString styled_text;
    int style_nr;
    const QsciStyle *explicit_style;
};

#ifdef __APPLE__
}
#endif

#endif
