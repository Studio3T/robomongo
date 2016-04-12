// Scintilla source code edit control
/** @file FontQuality.h
 ** Definitions to control font anti-aliasing.
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef FONTQUALITY_H
#define FONTQUALITY_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

#define SC_EFF_QUALITY_MASK            0xF
#define SC_EFF_QUALITY_DEFAULT           0
#define SC_EFF_QUALITY_NON_ANTIALIASED   1
#define SC_EFF_QUALITY_ANTIALIASED       2
#define SC_EFF_QUALITY_LCD_OPTIMIZED     3

#define SCWIN_TECH_GDI 0
#define SCWIN_TECH_DIRECTWRITE 1

#ifdef SCI_NAMESPACE
}
#endif

#endif
