// This module implements the QsciLexer class.
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


#include "Qsci/qscilexer.h"

#include <qapplication.h>
#include <qcolor.h>
#include <qfont.h>
#include <qsettings.h>

#include "Qsci/qsciapis.h"
#include "Qsci/qsciscintilla.h"
#include "Qsci/qsciscintillabase.h"


// The ctor.
QsciLexer::QsciLexer(QObject *parent, const char *name)
    : QObject(parent, name),
      autoIndStyle(-1), apiSet(0), attached_editor(0)
{
#if defined(Q_OS_WIN)
    defFont = QFont("Verdana",10);
#elif defined(Q_OS_MAC)
    defFont = QFont("Verdana", 12);
#else
    defFont = QFont("Bitstream Vera Sans",9);
#endif

    // Set the default fore and background colours.
    QColorGroup cg = QApplication::palette().active();
    defColor = cg.text();
    defPaper = cg.base();

    // Putting this on the heap means we can keep the style getters const.
    style_map = new StyleDataMap;
    style_map->style_data_set = false;
}


// The dtor.
QsciLexer::~QsciLexer()
{
    delete style_map;
}


// Set the attached editor.
void QsciLexer::setEditor(QsciScintilla *editor)
{
    attached_editor = editor;

    if (attached_editor)
    {
        attached_editor->SendScintilla(QsciScintillaBase::SCI_SETSTYLEBITS,
                styleBitsNeeded());
    }
}


// Return the lexer name.
const char *QsciLexer::lexer() const
{
    return 0;
}


// Return the lexer identifier.
int QsciLexer::lexerId() const
{
    return QsciScintillaBase::SCLEX_CONTAINER;
}


// Return the number of style bits needed by the lexer.
int QsciLexer::styleBitsNeeded() const
{
    if (!attached_editor)
        return 5;

    return attached_editor->SendScintilla(QsciScintillaBase::SCI_GETSTYLEBITSNEEDED);
}


// Make sure the style defaults have been set.
void QsciLexer::setStyleDefaults() const
{
    if (!style_map->style_data_set)
    {
        for (int i = 0; i < 128; ++i)
            if (!description(i).isEmpty())
                styleData(i);

        style_map->style_data_set = true;
    }
}


// Return a reference to a style's data, setting up the defaults if needed.
QsciLexer::StyleData &QsciLexer::styleData(int style) const
{
    StyleData &sd = style_map->style_data[style];

    // See if this is a new style by checking if the colour is valid.
    if (!sd.color.isValid())
    {
        sd.color = defaultColor(style);
        sd.paper = defaultPaper(style);
        sd.font = defaultFont(style);
        sd.eol_fill = defaultEolFill(style);
    }

    return sd;
}


// Set the APIs associated with the lexer.
void QsciLexer::setAPIs(QsciAbstractAPIs *apis)
{
    apiSet = apis;
}


// Return a pointer to the current APIs if there are any.
QsciAbstractAPIs *QsciLexer::apis() const
{
    return apiSet;
}


// Default implementation to return the set of fill up characters that can end
// auto-completion.
const char *QsciLexer::autoCompletionFillups() const
{
    return "(";
}


// Default implementation to return the view used for indentation guides.
int QsciLexer::indentationGuideView() const
{
    return QsciScintillaBase::SC_IV_LOOKBOTH;
}


// Default implementation to return the list of character sequences that can
// separate auto-completion words.
QStringList QsciLexer::autoCompletionWordSeparators() const
{
    return QStringList();
}


// Default implementation to return the list of keywords that can start a
// block.
const char *QsciLexer::blockStartKeyword(int *) const
{
    return 0;
}


// Default implementation to return the list of characters that can start a
// block.
const char *QsciLexer::blockStart(int *) const
{
    return 0;
}


// Default implementation to return the list of characters that can end a
// block.
const char *QsciLexer::blockEnd(int *) const
{
    return 0;
}


// Default implementation to return the style used for braces.
int QsciLexer::braceStyle() const
{
    return -1;
}


// Default implementation to return the number of lines to look back when
// auto-indenting.
int QsciLexer::blockLookback() const
{
    return 20;
}


// Default implementation to return the case sensitivity of the language.
bool QsciLexer::caseSensitive() const
{
    return true;
}


// Default implementation to return the characters that make up a word.
const char *QsciLexer::wordCharacters() const
{
    return 0;
}


// Default implementation to return the style used for whitespace.
int QsciLexer::defaultStyle() const
{
    return 0;
}


// Returns the foreground colour of the text for a style.
QColor QsciLexer::color(int style) const
{
    return styleData(style).color;
}


// Returns the background colour of the text for a style.
QColor QsciLexer::paper(int style) const
{
    return styleData(style).paper;
}


// Returns the font for a style.
QFont QsciLexer::font(int style) const
{
    return styleData(style).font;
}


// Returns the end-of-line fill for a style.
bool QsciLexer::eolFill(int style) const
{
    return styleData(style).eol_fill;
}


// Returns the set of keywords.
const char *QsciLexer::keywords(int) const
{
    return 0;
}


// Returns the default EOL fill for a style.
bool QsciLexer::defaultEolFill(int) const
{
    return false;
}


// Returns the default font for a style.
QFont QsciLexer::defaultFont(int) const
{
    return defaultFont();
}


// Returns the default font.
QFont QsciLexer::defaultFont() const
{
    return defFont;
}


// Sets the default font.
void QsciLexer::setDefaultFont(const QFont &f)
{
    defFont = f;
}


// Returns the default text colour for a style.
QColor QsciLexer::defaultColor(int) const
{
    return defaultColor();
}


// Returns the default text colour.
QColor QsciLexer::defaultColor() const
{
    return defColor;
}


// Sets the default text colour.
void QsciLexer::setDefaultColor(const QColor &c)
{
    defColor = c;
}


// Returns the default paper colour for a styles.
QColor QsciLexer::defaultPaper(int) const
{
    return defaultPaper();
}


// Returns the default paper colour.
QColor QsciLexer::defaultPaper() const
{
    return defPaper;
}


// Sets the default paper colour.
void QsciLexer::setDefaultPaper(const QColor &c)
{
    defPaper = c;

    // Normally the default values are only intended to provide defaults when a
    // lexer is first setup because once a style has been referenced then a
    // copy of the default is made.  However the default paper is a special
    // case because there is no other way to set the background colour used
    // where there is no text.  Therefore we also actively set it.
    setPaper(c, QsciScintillaBase::STYLE_DEFAULT);
}


// Read properties from the settings.
bool QsciLexer::readProperties(QSettings &,const QString &)
{
    return true;
}


// Refresh all properties.
void QsciLexer::refreshProperties()
{
}


// Write properties to the settings.
bool QsciLexer::writeProperties(QSettings &,const QString &) const
{
    return true;
}


// Restore the user settings.
bool QsciLexer::readSettings(QSettings &qs,const char *prefix)
{
    bool ok, flag, rc = true;
    int num;
    QString key, full_key;
    QStringList fdesc;

    setStyleDefaults();

    // Read the styles.
    for (int i = 0; i < 128; ++i)
    {
        // Ignore invalid styles.
        if (description(i).isEmpty())
            continue;

        key.sprintf("%s/%s/style%d/",prefix,language(),i);

        // Read the foreground colour.
        full_key = key + "color";

        num = qs.readNumEntry(full_key, 0, &ok);

        if (ok)
            setColor(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff), i);
        else
            rc = false;

        // Read the end-of-line fill.
        full_key = key + "eolfill";

        flag = qs.readBoolEntry(full_key, 0, &ok);

        if (ok)
            setEolFill(flag, i);
        else
            rc = false;

        // Read the font
        full_key = key + "font";

        fdesc = qs.readListEntry(full_key, ',', &ok);

        if (ok && fdesc.count() == 5)
        {
            QFont f;

            f.setFamily(fdesc[0]);
            f.setPointSize(fdesc[1].toInt());
            f.setBold(fdesc[2].toInt());
            f.setItalic(fdesc[3].toInt());
            f.setUnderline(fdesc[4].toInt());

            setFont(f, i);
        }
        else
            rc = false;

        // Read the background colour.
        full_key = key + "paper";

        num = qs.readNumEntry(full_key, 0, &ok);

        if (ok)
            setPaper(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff), i);
        else
            rc = false;
    }

    // Read the properties.
    key.sprintf("%s/%s/properties/",prefix,language());

    if (!readProperties(qs,key))
        rc = false;

    refreshProperties();

    // Read the rest.
    key.sprintf("%s/%s/",prefix,language());

    // Read the default foreground colour.
    full_key = key + "defaultcolor";

    num = qs.readNumEntry(full_key, 0, &ok);

    if (ok)
        setDefaultColor(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff));
    else
        rc = false;

    // Read the default background colour.
    full_key = key + "defaultpaper";

    num = qs.readNumEntry(full_key, 0, &ok);

    if (ok)
        setDefaultPaper(QColor((num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff));
    else
        rc = false;

    // Read the default font.
    full_key = key + "defaultfont";

    fdesc = qs.readListEntry(full_key, ',', &ok);

    if (ok && fdesc.count() == 5)
    {
        QFont f;

        f.setFamily(fdesc[0]);
        f.setPointSize(fdesc[1].toInt());
        f.setBold(fdesc[2].toInt());
        f.setItalic(fdesc[3].toInt());
        f.setUnderline(fdesc[4].toInt());

        setDefaultFont(f);
    }
    else
        rc = false;

    full_key = key + "autoindentstyle";

    num = qs.readNumEntry(full_key, 0, &ok);

    if (ok)
        setAutoIndentStyle(num);
    else
        rc = false;

    return rc;
}


// Save the user settings.
bool QsciLexer::writeSettings(QSettings &qs,const char *prefix) const
{
    bool rc = true;
    QString key, fmt("%1");
    int num;
    QStringList fdesc;

    setStyleDefaults();

    // Write the styles.
    for (int i = 0; i < 128; ++i)
    {
        // Ignore invalid styles.
        if (description(i).isEmpty())
            continue;

        QColor c;

        key.sprintf("%s/%s/style%d/",prefix,language(),i);

        // Write the foreground colour.
        c = color(i);
        num = (c.red() << 16) | (c.green() << 8) | c.blue();

        if (!qs.writeEntry(key + "color", num))
            rc = false;

        // Write the end-of-line fill.
        if (!qs.writeEntry(key + "eolfill", eolFill(i)))
            rc = false;

        // Write the font
        QFont f = font(i);

        fdesc.clear();
        fdesc += f.family();
        fdesc += fmt.arg(f.pointSize());

        // The casts are for Borland.
        fdesc += fmt.arg((int)f.bold());
        fdesc += fmt.arg((int)f.italic());
        fdesc += fmt.arg((int)f.underline());

        if (!qs.writeEntry(key + "font", fdesc, ','))
            rc = false;

        // Write the background colour.
        c = paper(i);
        num = (c.red() << 16) | (c.green() << 8) | c.blue();

        if (!qs.writeEntry(key + "paper", num))
            rc = false;
    }

    // Write the properties.
    key.sprintf("%s/%s/properties/",prefix,language());

    if (!writeProperties(qs,key))
        rc = false;

    // Write the rest.
    key.sprintf("%s/%s/",prefix,language());

    // Write the default foreground colour.
    num = (defColor.red() << 16) | (defColor.green() << 8) | defColor.blue();

    if (!qs.writeEntry(key + "defaultcolor", num))
        rc = false;

    // Write the default background colour.
    num = (defPaper.red() << 16) | (defPaper.green() << 8) | defPaper.blue();

    if (!qs.writeEntry(key + "defaultpaper", num))
        rc = false;

    // Write the default font
    fdesc.clear();
    fdesc += defFont.family();
    fdesc += fmt.arg(defFont.pointSize());

    // The casts are for Borland.
    fdesc += fmt.arg((int)defFont.bold());
    fdesc += fmt.arg((int)defFont.italic());
    fdesc += fmt.arg((int)defFont.underline());

    if (!qs.writeEntry(key + "defaultfont", fdesc, ','))
        rc = false;

    if (!qs.writeEntry(key + "autoindentstyle", autoIndStyle))
        rc = false;

    return rc;
}


// Return the auto-indentation style.
int QsciLexer::autoIndentStyle()
{
    // We can't do this in the ctor because we want the virtuals to work.
    if (autoIndStyle < 0)
        autoIndStyle = (blockStartKeyword() || blockStart() || blockEnd()) ?
                    0 : QsciScintilla::AiMaintain;

    return autoIndStyle;
}


// Set the auto-indentation style.
void QsciLexer::setAutoIndentStyle(int autoindentstyle)
{
    autoIndStyle = autoindentstyle;
}


// Set the foreground colour for a style.
void QsciLexer::setColor(const QColor &c, int style)
{
    if (style >= 0)
    {
        styleData(style).color = c;
        emit colorChanged(c, style);
    }
    else
        for (int i = 0; i < 128; ++i)
            if (!description(i).isEmpty())
                setColor(c, i);
}


// Set the end-of-line fill for a style.
void QsciLexer::setEolFill(bool eolfill, int style)
{
    if (style >= 0)
    {
        styleData(style).eol_fill = eolfill;
        emit eolFillChanged(eolfill, style);
    }
    else
        for (int i = 0; i < 128; ++i)
            if (!description(i).isEmpty())
                setEolFill(eolfill, i);
}


// Set the font for a style.
void QsciLexer::setFont(const QFont &f, int style)
{
    if (style >= 0)
    {
        styleData(style).font = f;
        emit fontChanged(f, style);
    }
    else
        for (int i = 0; i < 128; ++i)
            if (!description(i).isEmpty())
                setFont(f, i);
}


// Set the background colour for a style.
void QsciLexer::setPaper(const QColor &c, int style)
{
    if (style >= 0)
    {
        styleData(style).paper = c;
        emit paperChanged(c, style);
    }
    else
    {
        for (int i = 0; i < 128; ++i)
            if (!description(i).isEmpty())
                setPaper(c, i);

        emit paperChanged(c, QsciScintillaBase::STYLE_DEFAULT);
    }
}
