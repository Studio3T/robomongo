// This defines the interface to the QsciLexerTeX class.
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


#ifndef QSCILEXERTEX_H
#define QSCILEXERTEX_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>


//! \brief The QsciLexerTeX class encapsulates the Scintilla TeX lexer.
class QSCINTILLA_EXPORT QsciLexerTeX : public QsciLexer
{
    Q_OBJECT

public:
    //! This enum defines the meanings of the different styles used by the
    //! TeX lexer.
    enum {
        //! The default.
        Default = 0,

        //! A special.
        Special = 1,

        //! A group.
        Group = 2,

        //! A symbol.
        Symbol = 3,

        //! A command.
        Command = 4,

        //! Text.
        Text = 5
    };

    //! Construct a QsciLexerTeX with parent \a parent.  \a parent is typically
    //! the QsciScintilla instance.
    QsciLexerTeX(QObject *parent = 0);

    //! Destroys the QsciLexerTeX instance.
    virtual ~QsciLexerTeX();

    //! Returns the name of the language.
    const char *language() const;

    //! Returns the name of the lexer.  Some lexers support a number of
    //! languages.
    const char *lexer() const;

    //! Returns the string of characters that comprise a word.
    const char *wordCharacters() const;

    //! Returns the foreground colour of the text for style number \a style.
    QColor defaultColor(int style) const;

    //! Returns the set of keywords for the keyword set \a set recognised
    //! by the lexer as a space separated string.
    const char *keywords(int set) const;

    //! Returns the descriptive name for style number \a style.  If the
    //! style is invalid for this language then an empty QString is returned.
    //! This is intended to be used in user preference dialogs.
    QString description(int style) const;

    //! Causes all properties to be refreshed by emitting the
    //! propertyChanged() signal as required.
    void refreshProperties();

    //! If \a fold is true then multi-line comment blocks can be folded.  The
    //! default is false.
    //!
    //! \sa foldComments()
    void setFoldComments(bool fold);

    //! Returns true if multi-line comment blocks can be folded.
    //!
    //! \sa setFoldComments()
    bool foldComments() const {return fold_comments;}

    //! If \a fold is true then trailing blank lines are included in a fold
    //! block. The default is true.
    //!
    //! \sa foldCompact()
    void setFoldCompact(bool fold);

    //! Returns true if trailing blank lines are included in a fold block.
    //!
    //! \sa setFoldCompact()
    bool foldCompact() const {return fold_compact;}

    //! If \a enable is true then comments are processed as TeX source
    //! otherwise they are ignored.  The default is false.
    //!
    //! \sa processComments()
    void setProcessComments(bool enable);

    //! Returns true if comments are processed as TeX source.
    //!
    //! \sa setProcessComments()
    bool processComments() const {return process_comments;}

    //! If \a enable is true then \\if<unknown> processed is processed as a
    //! command.  The default is true.
    //!
    //! \sa processIf()
    void setProcessIf(bool enable);

    //! Returns true if \\if<unknown> is processed as a command.
    //!
    //! \sa setProcessIf()
    bool processIf() const {return process_if;}

protected:
    //! The lexer's properties are read from the settings \a qs.  \a prefix
    //! (which has a trailing '/') should be used as a prefix to the key of
    //! each setting.  true is returned if there is no error.
    //!
    bool readProperties(QSettings &qs, const QString &prefix);

    //! The lexer's properties are written to the settings \a qs.
    //! \a prefix (which has a trailing '/') should be used as a prefix to
    //! the key of each setting.  true is returned if there is no error.
    //!
    bool writeProperties(QSettings &qs, const QString &prefix) const;

private:
    void setCommentProp();
    void setCompactProp();
    void setProcessCommentsProp();
    void setAutoIfProp();

    bool fold_comments;
    bool fold_compact;
    bool process_comments;
    bool process_if;

    QsciLexerTeX(const QsciLexerTeX &);
    QsciLexerTeX &operator=(const QsciLexerTeX &);
};

#ifdef __APPLE__
}
#endif

#endif
