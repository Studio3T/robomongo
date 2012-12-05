// This defines the interface to the QsciLexerSpice class.
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


#ifndef QSCILEXERSPICE_H
#define QSCILEXERSPICE_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>


//! \brief The QsciLexerSpice class encapsulates the Scintilla Spice lexer.
class QSCINTILLA_EXPORT QsciLexerSpice : public QsciLexer
{
    Q_OBJECT

public:
    //! This enum defines the meanings of the different styles used by the
    //! Spice lexer.
    enum {
        //! The default.
        Default = 0,

        //! An identifier.
        Identifier = 1,

        //! A command.
        Command = 2,

        //! A function.
        Function = 3,

        //! A parameter.
        Parameter = 4,

        //! A number.
        Number = 5,

        //! A delimiter.
        Delimiter = 6,

        //! A value.
        Value = 7,

        //! A comment.
        Comment = 8
    };

    //! Construct a QsciLexerSpice with parent \a parent.  \a parent is
    //! typically the QsciScintilla instance.
    QsciLexerSpice(QObject *parent = 0);

    //! Destroys the QsciLexerSpice instance.
    virtual ~QsciLexerSpice();

    //! Returns the name of the language.
    const char *language() const;

    //! Returns the name of the lexer.  Some lexers support a number of
    //! languages.
    const char *lexer() const;

    //! \internal Returns the style used for braces for brace matching.
    int braceStyle() const;

    //! Returns the set of keywords for the keyword set \a set recognised
    //! by the lexer as a space separated string.
    const char *keywords(int set) const;

    //! Returns the foreground colour of the text for style number \a style.
    //!
    //! \sa defaultPaper()
    QColor defaultColor(int style) const;

    //! Returns the font for style number \a style.
    QFont defaultFont(int style) const;

    //! Returns the descriptive name for style number \a style.  If the
    //! style is invalid for this language then an empty QString is returned.
    //! This is intended to be used in user preference dialogs.
    QString description(int style) const;

private:
    QsciLexerSpice(const QsciLexerSpice &);
    QsciLexerSpice &operator=(const QsciLexerSpice &);
};

#ifdef __APPLE__
}
#endif

#endif
