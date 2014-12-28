// This module defines various things common to all of the Scintilla Qt port.
//
// Copyright (c) 2014 Riverbank Computing Limited <info@riverbankcomputing.com>
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


#ifndef QSCIGLOBAL_H
#define QSCIGLOBAL_H

#ifdef __APPLE__
extern "C++" {
#endif

#include <qglobal.h>


#define QSCINTILLA_VERSION      0x020804
#define QSCINTILLA_VERSION_STR  "2.8.4"


// Under Windows, define QSCINTILLA_MAKE_DLL to create a Scintilla DLL, or
// define QSCINTILLA_DLL to link against a Scintilla DLL, or define neither
// to either build or link against a static Scintilla library.
#if defined(Q_OS_WIN)

#if defined(QSCINTILLA_DLL)
#define QSCINTILLA_EXPORT       __declspec(dllimport)
#elif defined(QSCINTILLA_MAKE_DLL)
#define QSCINTILLA_EXPORT       __declspec(dllexport)
#endif

#endif

#if !defined(QSCINTILLA_EXPORT)
#define QSCINTILLA_EXPORT
#endif


#if !defined(QT_BEGIN_NAMESPACE)
#define QT_BEGIN_NAMESPACE
#define QT_END_NAMESPACE
#endif

#ifdef __APPLE__
}
#endif

#endif
