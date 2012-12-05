// This defines the interface to the QsciLexerDiff class.
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


#ifndef QSCILEXERDIFF_H
#define QSCILEXERDIFF_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>


//! \brief The QsciLexerDiff class encapsulates the Scintilla Diff
//! lexer.
class QSCINTILLA_EXPORT QsciLexerDiff : public QsciLexer
{
    Q_OBJECT

public:
    //! This enum defines the meanings of the different styles used by the
    //! Diff lexer.
    enum {
        //! The default.
        Default = 0,

        //! A comment.
        Comment = 1,

        //! A command.
        Command = 2,

        //! A header.
        Header = 3,

        //! A position.
        Position = 4,

        //! A removed line.
        LineRemoved = 5,

        //! An added line.
        LineAdded = 6,

        //! A changed line.
        LineChanged = 7
    };

    //! Construct a QsciLexerDiff with parent \a parent.  \a parent is
    //! typically the QsciScintilla instance.
    QsciLexerDiff(QObject *parent = 0);

    //! Destroys the QsciLexerDiff instance.
    virtual ~QsciLexerDiff();

    //! Returns the name of the language.
    const char *language() const;

    //! Returns the name of the lexer.  Some lexers support a number of
    //! languages.
    const char *lexer() const;

    //! Returns the string of characters that comprise a word.
    const char *wordCharacters() const;

    //! Returns the foreground colour of the text for style number \a style.
    QColor defaultColor(int style) const;

    //! Returns the descriptive name for style number \a style.  If the
    //! style is invalid for this language then an empty QString is returned.
    //! This is intended to be used in user preference dialogs.
    QString description(int style) const;

private:
    QsciLexerDiff(const QsciLexerDiff &);
    QsciLexerDiff &operator=(const QsciLexerDiff &);
};

#ifdef __APPLE__
}
#endif

#endif
