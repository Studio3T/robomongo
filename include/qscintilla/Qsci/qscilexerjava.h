// This defines the interface to the QsciLexerJava class.
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


#ifndef QSCILEXERJAVA_H
#define QSCILEXERJAVA_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexercpp.h>


//! \brief The QsciLexerJava class encapsulates the Scintilla Java lexer.
class QSCINTILLA_EXPORT QsciLexerJava : public QsciLexerCPP
{
    Q_OBJECT

public:
    //! Construct a QsciLexerJava with parent \a parent.  \a parent is
    //! typically the QsciScintilla instance.
    QsciLexerJava(QObject *parent = 0);

    //! Destroys the QsciLexerJava instance.
    virtual ~QsciLexerJava();

    //! Returns the name of the language.
    const char *language() const;

    //! Returns the set of keywords for the keyword set \a set recognised
    //! by the lexer as a space separated string.
    const char *keywords(int set) const;

private:
    QsciLexerJava(const QsciLexerJava &);
    QsciLexerJava &operator=(const QsciLexerJava &);
};

#ifdef __APPLE__
}
#endif

#endif
