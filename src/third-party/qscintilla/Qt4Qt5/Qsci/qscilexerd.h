// This defines the interface to the QsciLexerD class.
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


#ifndef QSCILEXERD_H
#define QSCILEXERD_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>


//! \brief The QsciLexerD class encapsulates the Scintilla D lexer.
class QSCINTILLA_EXPORT QsciLexerD : public QsciLexer
{
    Q_OBJECT

public:
    //! This enum defines the meanings of the different styles used by the D
    //! lexer.
    enum {
        //! The default.
        Default = 0,

        //! A comment.
        Comment = 1,

        //! A comment line.
        CommentLine = 2,

        //! A JavaDoc and Doxygen comment.
        CommentDoc = 3,

        //! A nested comment.
        CommentNested = 4,

        //! A number.
        Number = 5,

        //! A keyword.
        Keyword = 6,

        //! A secondary keyword.
        KeywordSecondary = 7,

        //! A doc keyword
        KeywordDoc = 8,
        
        //! Typedefs and aliases
        Typedefs = 9,
        
        //! A string.
        String = 10,

        //! The end of a line where a string is not closed.
        UnclosedString = 11,
        
        //! A character
        Character = 12,

        //! An operator.
        Operator = 13,

        //! An identifier
        Identifier = 14,

        //! A JavaDoc and Doxygen line.
        CommentLineDoc = 15,

        //! A JavaDoc and Doxygen  keyword.
        CommentDocKeyword = 16,

        //! A JavaDoc and Doxygen keyword error.
        CommentDocKeywordError = 17,

        //! A backquoted string.
        BackquoteString = 18,

        //! A raw, hexadecimal or delimited string.
        RawString = 19,

        //! A keyword defined in keyword set number 5.  The class must be
        //! sub-classed and re-implement keywords() to make use of this style.
        KeywordSet5 = 20,

        //! A keyword defined in keyword set number 6.  The class must be
        //! sub-classed and re-implement keywords() to make use of this style.
        KeywordSet6 = 21,

        //! A keyword defined in keyword set number 7.  The class must be
        //! sub-classed and re-implement keywords() to make use of this style.
        KeywordSet7 = 22,
    };

    //! Construct a QsciLexerD with parent \a parent.  \a parent is typically
    //! the QsciScintilla instance.
    QsciLexerD(QObject *parent = 0);

    //! Destroys the QsciLexerD instance.
    virtual ~QsciLexerD();

    //! Returns the name of the language.
    const char *language() const;

    //! Returns the name of the lexer.  Some lexers support a number of
    //! languages.
    const char *lexer() const;

    //! \internal Returns the character sequences that can separate
    //! auto-completion words.
    QStringList autoCompletionWordSeparators() const;

    //! \internal Returns a space separated list of words or characters in a
    //! particular style that define the end of a block for auto-indentation.
    //! The styles is returned via \a style.
    const char *blockEnd(int *style = 0) const;

    //! \internal Returns a space separated list of words or characters in a
    //! particular style that define the start of a block for auto-indentation.
    //! The styles is returned via \a style.
    const char *blockStart(int *style = 0) const;

    //! \internal Returns a space separated list of keywords in a particular
    //! style that define the start of a block for auto-indentation.  The
    //! styles is returned via \a style.
    const char *blockStartKeyword(int *style = 0) const;

    //! \internal Returns the style used for braces for brace matching.
    int braceStyle() const;

    //! Returns the string of characters that comprise a word.
    const char *wordCharacters() const;

    //! Returns the foreground colour of the text for style number \a style.
    //!
    //! \sa defaultPaper()
    QColor defaultColor(int style) const;

    //! Returns the end-of-line fill for style number \a style.
    bool defaultEolFill(int style) const;

    //! Returns the font for style number \a style.
    QFont defaultFont(int style) const;

    //! Returns the background colour of the text for style number \a style.
    //!
    //! \sa defaultColor()
    QColor defaultPaper(int style) const;

    //! Returns the set of keywords for the keyword set \a set recognised by
    //! the lexer as a space separated string.
    const char *keywords(int set) const;

    //! Returns the descriptive name for style number \a style.  If the style
    //! is invalid for this language then an empty QString is returned.  This
    //! is intended to be used in user preference dialogs.
    QString description(int style) const;

    //! Causes all properties to be refreshed by emitting the propertyChanged()
    //! signal as required.
    void refreshProperties();

    //! Returns true if "} else {" lines can be folded.
    //!
    //! \sa setFoldAtElse()
    bool foldAtElse() const;

    //! Returns true if multi-line comment blocks can be folded.
    //!
    //! \sa setFoldComments()
    bool foldComments() const;

    //! Returns true if trailing blank lines are included in a fold block.
    //!
    //! \sa setFoldCompact()
    bool foldCompact() const;

public slots:
    //! If \a fold is true then "} else {" lines can be folded.  The default is
    //! false.
    //!
    //! \sa foldAtElse()
    virtual void setFoldAtElse(bool fold);

    //! If \a fold is true then multi-line comment blocks can be folded.  The
    //! default is false.
    //!
    //! \sa foldComments()
    virtual void setFoldComments(bool fold);

    //! If \a fold is true then trailing blank lines are included in a fold
    //! block. The default is true.
    //!
    //! \sa foldCompact()
    virtual void setFoldCompact(bool fold);

protected:
    //! The lexer's properties are read from the settings \a qs.  \a prefix
    //! (which has a trailing '/') should be used as a prefix to the key of
    //! each setting.  true is returned if there is no error.
    //!
    //! \sa writeProperties()
    bool readProperties(QSettings &qs,const QString &prefix);

    //! The lexer's properties are written to the settings \a qs.  \a prefix
    //! (which has a trailing '/') should be used as a prefix to the key of
    //! each setting.  true is returned if there is no error.
    //!
    //! \sa readProperties()
    bool writeProperties(QSettings &qs,const QString &prefix) const;

private:
    void setAtElseProp();
    void setCommentProp();
    void setCompactProp();

    bool fold_atelse;
    bool fold_comments;
    bool fold_compact;

    QsciLexerD(const QsciLexerD &);
    QsciLexerD &operator=(const QsciLexerD &);
};

#ifdef __APPLE__
}
#endif

#endif
