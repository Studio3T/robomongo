// This module implements the QsciLexerXML class.
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


#include "Qsci/qscilexerxml.h"

#include <qcolor.h>
#include <qfont.h>
#include <qsettings.h>


// The ctor.
QsciLexerXML::QsciLexerXML(QObject *parent)
    : QsciLexerHTML(parent),
      scripts(true)
{
}


// The dtor.
QsciLexerXML::~QsciLexerXML()
{
}


// Returns the language name.
const char *QsciLexerXML::language() const
{
    return "XML";
}


// Returns the lexer name.
const char *QsciLexerXML::lexer() const
{
    return "xml";
}


// Returns the foreground colour of the text for a style.
QColor QsciLexerXML::defaultColor(int style) const
{
    switch (style)
    {
    case Default:
        return QColor(0x00,0x00,0x00);

    case Tag:
    case UnknownTag:
    case XMLTagEnd:
    case SGMLDefault:
    case SGMLCommand:
        return QColor(0x00,0x00,0x80);

    case Attribute:
    case UnknownAttribute:
        return QColor(0x00,0x80,0x80);

    case HTMLNumber:
        return QColor(0x00,0x7f,0x7f);

    case HTMLDoubleQuotedString:
    case HTMLSingleQuotedString:
        return QColor(0x7f,0x00,0x7f);

    case OtherInTag:
    case Entity:
    case XMLStart:
    case XMLEnd:
        return QColor(0x80,0x00,0x80);

    case HTMLComment:
    case SGMLComment:
        return QColor(0x80,0x80,0x00);

    case CDATA:
    case PHPStart:
    case SGMLDoubleQuotedString:
    case SGMLError:
        return QColor(0x80,0x00,0x00);

    case HTMLValue:
        return QColor(0x60,0x80,0x60);

    case SGMLParameter:
        return QColor(0x00,0x66,0x00);

    case SGMLSingleQuotedString:
        return QColor(0x99,0x33,0x00);

    case SGMLSpecial:
        return QColor(0x33,0x66,0xff);

    case SGMLEntity:
        return QColor(0x33,0x33,0x33);

    case SGMLBlockDefault:
        return QColor(0x00,0x00,0x66);
    }

    return QsciLexerHTML::defaultColor(style);
}


// Returns the end-of-line fill for a style.
bool QsciLexerXML::defaultEolFill(int style) const
{
    if (style == CDATA)
        return true;

    return QsciLexerHTML::defaultEolFill(style);
}


// Returns the font of the text for a style.
QFont QsciLexerXML::defaultFont(int style) const
{
    QFont f;

    switch (style)
    {
    case Default:
    case Entity:
    case CDATA:
#if defined(Q_OS_WIN)
        f = QFont("Times New Roman",11);
#elif defined(Q_OS_MAC)
        f = QFont("Times New Roman", 12);
#else
        f = QFont("Bitstream Charter",10);
#endif
        break;

    case XMLStart:
    case XMLEnd:
    case SGMLCommand:
        f = QsciLexer::defaultFont(style);
        f.setBold(true);
        break;

    default:
        f = QsciLexerHTML::defaultFont(style);
    }

    return f;
}


// Returns the set of keywords.
const char *QsciLexerXML::keywords(int set) const
{
    if (set == 6)
        return QsciLexerHTML::keywords(set);

    return 0;
}


// Returns the background colour of the text for a style.
QColor QsciLexerXML::defaultPaper(int style) const
{
    switch (style)
    {
    case CDATA:
        return QColor(0xff,0xf0,0xf0);

    case SGMLDefault:
    case SGMLCommand:
    case SGMLParameter:
    case SGMLDoubleQuotedString:
    case SGMLSingleQuotedString:
    case SGMLSpecial:
    case SGMLEntity:
    case SGMLComment:
        return QColor(0xef,0xef,0xff);

    case SGMLError:
        return QColor(0xff,0x66,0x66);

    case SGMLBlockDefault:
        return QColor(0xcc,0xcc,0xe0);
    }

    return QsciLexerHTML::defaultPaper(style);
}


// Refresh all properties.
void QsciLexerXML::refreshProperties()
{
    setScriptsProp();
}


// Read properties from the settings.
bool QsciLexerXML::readProperties(QSettings &qs, const QString &prefix)
{
    int rc = QsciLexerHTML::readProperties(qs, prefix), num;

    scripts = qs.value(prefix + "scriptsstyled", true).toBool();

    return rc;
}


// Write properties to the settings.
bool QsciLexerXML::writeProperties(QSettings &qs, const QString &prefix) const
{
    int rc = QsciLexerHTML::writeProperties(qs, prefix);

    qs.setValue(prefix + "scriptsstyled", scripts);

    return rc;
}


// Return true if scripts are styled.
bool QsciLexerXML::scriptsStyled() const
{
    return scripts;
}


// Set if scripts are styled.
void QsciLexerXML::setScriptsStyled(bool styled)
{
    scripts = styled;

    setScriptsProp();
}


// Set the "lexer.xml.allow.scripts" property.
void QsciLexerXML::setScriptsProp()
{
    emit propertyChanged("lexer.xml.allow.scripts",(scripts ? "1" : "0"));
}
