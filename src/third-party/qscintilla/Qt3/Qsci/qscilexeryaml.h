// This defines the interface to the QsciLexerYAML class.
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


#ifndef QSCILEXERYAML_H
#define QSCILEXERYAML_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qobject.h>

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>


//! \brief The QsciLexerYAML class encapsulates the Scintilla YAML lexer.
class QSCINTILLA_EXPORT QsciLexerYAML : public QsciLexer
{
    Q_OBJECT

public:
    //! This enum defines the meanings of the different styles used by the
    //! YAML lexer.
    enum {
        //! The default.
        Default = 0,

        //! A comment.
        Comment = 1,

        //! An identifier.
        Identifier = 2,

        //! A keyword
        Keyword = 3,

        //! A number.
        Number = 4,

        //! A reference.
        Reference = 5,

        //! A document delimiter.
        DocumentDelimiter = 6,

        //! A text block marker.
        TextBlockMarker = 7,

        //! A syntax error marker.
        SyntaxErrorMarker = 8,

        //! An operator.
        Operator = 9
    };

    //! Construct a QsciLexerYAML with parent \a parent and name \a name.  \a
    //! parent is typically the QsciScintilla instance.
    QsciLexerYAML(QObject *parent = 0, const char *name = 0);

    //! Destroys the QsciLexerYAML instance.
    virtual ~QsciLexerYAML();

    //! Returns the name of the language.
    const char *language() const;

    //! Returns the name of the lexer.  Some lexers support a number of
    //! languages.
    const char *lexer() const;

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

    //! Returns the set of keywords for the keyword set \a set recognised
    //! by the lexer as a space separated string.
    const char *keywords(int set) const;

    //! Returns the descriptive name for style number \a style.  If the
    //! style is invalid for this language then an empty QString is returned.
    //! This is intended to be used in user preference dialogs.
    QString description(int style) const;

    //! Causes all properties to be refreshed by emitting the propertyChanged()
    //! signal as required.
    void refreshProperties();

    //! Returns true if multi-line comment blocks can be folded.
    //!
    //! \sa setFoldComments()
    bool foldComments() const;

public slots:
    //! If \a fold is true then multi-line comment blocks can be folded.
    //! The default is false.
    //!
    //! \sa foldComments()
    virtual void setFoldComments(bool fold);

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
    void setCommentProp();

    bool fold_comments;

    QsciLexerYAML(const QsciLexerYAML &);
    QsciLexerYAML &operator=(const QsciLexerYAML &);
};

#ifdef __APPLE__
}
#endif

#endif
