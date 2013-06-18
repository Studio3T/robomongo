// This module defines interface to the QsciStyle class.
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


#ifndef QSCISTYLE_H
#define QSCISTYLE_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qcolor.h>
#include <qfont.h>
#include <qstring.h>

#include <Qsci/qsciglobal.h>


class QsciScintillaBase;


//! \brief The QsciStyle class encapsulates all the attributes of a style.
//!
//! Each character of a document has an associated style which determines how
//! the character is displayed, e.g. its font and color.  A style is identified
//! by a number.  Lexers define styles for each of the language's features so
//! that they are displayed differently.  Some style numbers have hard-coded
//! meanings, e.g. the style used for call tips.
class QSCINTILLA_EXPORT QsciStyle
{
public:
    //! This enum defines the different ways the displayed case of the text can
    //! be changed.
    enum TextCase {
        //! The text is displayed as its original case.
        OriginalCase = 0,

        //! The text is displayed as upper case.
        UpperCase = 1,

        //! The text is displayed as lower case.
        LowerCase = 2
    };

    //! Constructs a QsciStyle instance for style number \a style.  If \a style
    //! is negative then a new style number is automatically allocated.
    QsciStyle(int style = -1);

    //! Constructs a QsciStyle instance for style number \a style.  If \a style
    //! is negative then a new style number is automatically allocated.  The
    //! styles description, color, paper color, font and end-of-line fill are
    //! set to \a description, \a color, \a paper, \a font and \a eolFill
    //! respectively.
    QsciStyle(int style, const QString &description, const QColor &color,
            const QColor &paper, const QFont &font, bool eolFill = false);

    //! \internal Apply the style to a particular editor.
    void apply(QsciScintillaBase *sci) const;

    //! Returns the number of the style.
    int style() const {return style_nr;}

    //! The style's description is set to \a description.
    //!
    //! \sa description()
    void setDescription(const QString &description) {style_description = description;}

    //! Returns the style's description.
    //!
    //! \sa setDescription()
    QString description() const {return style_description;}

    //! The style's foreground color is set to \a color.  The default is taken
    //! from the application's default palette.
    //!
    //! \sa color()
    void setColor(const QColor &color);

    //! Returns the style's foreground color.
    //!
    //! \sa setColor()
    QColor color() const {return style_color;}

    //! The style's background color is set to \a paper.  The default is taken
    //! from the application's default palette.
    //!
    //! \sa paper()
    void setPaper(const QColor &paper);

    //! Returns the style's background color.
    //!
    //! \sa setPaper()
    QColor paper() const {return style_paper;}

    //! The style's font is set to \a font.  The default is the application's
    //! default font.
    //!
    //! \sa font()
    void setFont(const QFont &font);

    //! Returns the style's font.
    //!
    //! \sa setFont()
    QFont font() const {return style_font;}

    //! The style's end-of-line fill is set to \a fill.  The default is false.
    //!
    //! \sa eolFill()
    void setEolFill(bool fill);

    //! Returns the style's end-of-line fill.
    //!
    //! \sa setEolFill()
    bool eolFill() const {return style_eol_fill;}

    //! The style's text case is set to \a text_case.  The default is
    //! OriginalCase.
    //!
    //! \sa textCase()
    void setTextCase(TextCase text_case);

    //! Returns the style's text case.
    //!
    //! \sa setTextCase()
    TextCase textCase() const {return style_case;}

    //! The style's visibility is set to \a visible.  The default is true.
    //!
    //! \sa visible()
    void setVisible(bool visible);

    //! Returns the style's visibility.
    //!
    //! \sa setVisible()
    bool visible() const {return style_visible;}

    //! The style's changeability is set to \a changeable.  The default is
    //! true.
    //!
    //! \sa changeable()
    void setChangeable(bool changeable);

    //! Returns the style's changeability.
    //!
    //! \sa setChangeable()
    bool changeable() const {return style_changeable;}

    //! The style's sensitivity to mouse clicks is set to \a hotspot.  The
    //! default is false.
    //!
    //! \sa hotspot()
    void setHotspot(bool hotspot);

    //! Returns the style's sensitivity to mouse clicks.
    //!
    //! \sa setHotspot()
    bool hotspot() const {return style_hotspot;}

    //! Refresh the style settings.
    void refresh();

private:
    int style_nr;
    QString style_description;
    QColor style_color;
    QColor style_paper;
    QFont style_font;
    bool style_eol_fill;
    TextCase style_case;
    bool style_visible;
    bool style_changeable;
    bool style_hotspot;

    void init(int style);
};

#ifdef __APPLE__
}
#endif

#endif
