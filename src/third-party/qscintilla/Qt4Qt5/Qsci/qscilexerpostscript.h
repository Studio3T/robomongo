// This defines the interface to the QsciLexerPostScript class.
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


#ifndef QSCILEXERPOSTSCRIPT_H
#define QSCILEXERPOSTSCRIPT_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>


//! \brief The QsciLexerPostScript class encapsulates the Scintilla PostScript
//! lexer.
class QSCINTILLA_EXPORT QsciLexerPostScript : public QsciLexer
{
    Q_OBJECT

public:
    //! This enum defines the meanings of the different styles used by the
    //! PostScript lexer.
    enum {
        //! The default.
        Default = 0,

        //! A comment.
        Comment = 1,

        //! A DSC comment.
        DSCComment = 2,

        //! A DSC comment value.
        DSCCommentValue = 3,

        //! A number.
        Number = 4,

        //! A name.
        Name = 5,

        //! A keyword.
        Keyword = 6,

        //! A literal.
        Literal = 7,

        //! An immediately evaluated literal.
        ImmediateEvalLiteral = 8,

        //! Array parenthesis.
        ArrayParenthesis = 9,

        //! Dictionary parenthesis.
        DictionaryParenthesis = 10,

        //! Procedure parenthesis.
        ProcedureParenthesis = 11,

        //! Text.
        Text = 12,

        //! A hexadecimal string.
        HexString = 13,

        //! A base85 string.
        Base85String = 14,

        //! A bad string character.
        BadStringCharacter = 15
    };

    //! Construct a QsciLexerPostScript with parent \a parent.  \a parent is
    //! typically the QsciScintilla instance.
    QsciLexerPostScript(QObject *parent = 0);

    //! Destroys the QsciLexerPostScript instance.
    virtual ~QsciLexerPostScript();

    //! Returns the name of the language.
    const char *language() const;

    //! Returns the name of the lexer.  Some lexers support a number of
    //! languages.
    const char *lexer() const;

    //! \internal Returns the style used for braces for brace matching.
    int braceStyle() const;

    //! Returns the foreground colour of the text for style number \a style.
    //!
    //! \sa defaultPaper()
    QColor defaultColor(int style) const;

    //! Returns the font for style number \a style.
    QFont defaultFont(int style) const;

    //! Returns the background colour of the text for style number \a style.
    //!
    //! \sa defaultColor()
    QColor defaultPaper(int style) const;

    //! Returns the set of keywords for the keyword set \a set recognised
    //! by the lexer as a space separated string.  Set 5 can be used to provide
    //! additional user defined keywords.
    const char *keywords(int set) const;

    //! Returns the descriptive name for style number \a style.  If the
    //! style is invalid for this language then an empty QString is returned.
    //! This is intended to be used in user preference dialogs.
    QString description(int style) const;

    //! Causes all properties to be refreshed by emitting the propertyChanged()
    //! signal as required.
    void refreshProperties();

    //! Returns true if tokens should be marked.
    //!
    //! \sa setTokenize()
    bool tokenize() const;

    //! Returns the PostScript level.
    //!
    //! \sa setLevel()
    int level() const;

    //! Returns true if trailing blank lines are included in a fold block.
    //!
    //! \sa setFoldCompact()
    bool foldCompact() const;

    //! Returns true if else blocks can be folded.
    //!
    //! \sa setFoldAtElse()
    bool foldAtElse() const;

public slots:
    //! If \a tokenize is true then tokens are marked.  The default is false.
    //!
    //! \sa tokenize()
    virtual void setTokenize(bool tokenize);

    //! The PostScript level is set to \a level.  The default is 3.
    //!
    //! \sa level()
    virtual void setLevel(int level);

    //! If \a fold is true then trailing blank lines are included in a fold
    //! block. The default is true.
    //!
    //! \sa foldCompact()
    virtual void setFoldCompact(bool fold);

    //! If \a fold is true then else blocks can be folded.  The default is
    //! false.
    //!
    //! \sa foldAtElse()
    virtual void setFoldAtElse(bool fold);

protected:
    //! The lexer's properties are read from the settings \a qs.  \a prefix
    //! (which has a trailing '/') should be used as a prefix to the key of
    //! each setting.  true is returned if there is no error.
    //!
    bool readProperties(QSettings &qs,const QString &prefix);

    //! The lexer's properties are written to the settings \a qs.
    //! \a prefix (which has a trailing '/') should be used as a prefix to
    //! the key of each setting.  true is returned if there is no error.
    //!
    bool writeProperties(QSettings &qs,const QString &prefix) const;

private:
    void setTokenizeProp();
    void setLevelProp();
    void setCompactProp();
    void setAtElseProp();

    bool ps_tokenize;
    int ps_level;
    bool fold_compact;
    bool fold_atelse;

    QsciLexerPostScript(const QsciLexerPostScript &);
    QsciLexerPostScript &operator=(const QsciLexerPostScript &);
};

#ifdef __APPLE__
}
#endif

#endif
