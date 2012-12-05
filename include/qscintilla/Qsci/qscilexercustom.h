// This defines the interface to the QsciLexerCustom class.
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


#ifndef QSCILEXERCUSTOM_H
#define QSCILEXERCUSTOM_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>


class QsciScintilla;
class QsciStyle;


//! \brief The QsciLexerCustom class is an abstract class used as a base for
//! new language lexers.
//!
//! The advantage of implementing a new lexer this way (as opposed to adding
//! the lexer to the underlying Scintilla code) is that it does not require the
//! QScintilla library to be re-compiled.  It also makes it possible to
//! integrate external lexers.
//!
//! All that is necessary to implement a new lexer is to define appropriate
//! styles and to re-implement the styleText() method.
class QSCINTILLA_EXPORT QsciLexerCustom : public QsciLexer
{
    Q_OBJECT

public:
    //! Construct a QsciLexerCustom with parent \a parent.  \a parent is
    //! typically the QsciScintilla instance.
    QsciLexerCustom(QObject *parent = 0);

    //! Destroy the QSciLexerCustom.
    virtual ~QsciLexerCustom();

    //! The next \a length characters starting from the current styling
    //! position have their style set to style number \a style.  The current
    //! styling position is moved.  The styling position is initially set by
    //! calling startStyling().
    //!
    //! \sa startStyling(), styleText()
    void setStyling(int length, int style);

    //! The next \a length characters starting from the current styling
    //! position have their style set to style \a style.  The current styling
    //! position is moved.  The styling position is initially set by calling
    //! startStyling().
    //!
    //! \sa startStyling(), styleText()
    void setStyling(int length, const QsciStyle &style);

    //! The styling position is set to \a start and the mask of style bits that
    //! can be set is set to \a styleBits.  \a styleBits allows the styling of
    //! text to be done over several passes by setting different style bits on
    //! each pass.  If \a styleBits is 0 then all style bits (as returned by
    //! styleBitsNeeded()) are set.
    //!
    //! \sa setStyling(), styleBitsNeeded(), styleText()
    void startStyling(int pos, int styleBits = 0);

    //! This is called when the section of text beginning at position \a start
    //! and up to position \a end needs to be styled.  \a start will always be
    //! at the start of a line.  The text is styled by calling startStyling()
    //! followed by one or more calls to setStyling().  It must be
    //! re-implemented by a sub-class.
    //!
    //! \sa setStyling(), startStyling()
    virtual void styleText(int start, int end) = 0;

    //! \reimp
    virtual void setEditor(QsciScintilla *editor);

    //! \reimp This re-implementation returns 5 as the number of style bits
    //! needed.
    virtual int styleBitsNeeded() const;

private slots:
    void handleStyleNeeded(int pos);

private:
    QsciLexerCustom(const QsciLexerCustom &);
    QsciLexerCustom &operator=(const QsciLexerCustom &);
};

#ifdef __APPLE__
}
#endif

#endif
