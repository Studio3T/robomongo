// Support for building the Scintilla code in the Scintilla namespace using the
// -DSCI_NAMESPACE compiler flag.
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


#ifndef _SCINAMESPACE_H
#define _SCINAMESPACE_H

#ifdef SCI_NAMESPACE
#define QSCI_SCI_NAMESPACE(name)    Scintilla::name
#define QSCI_BEGIN_SCI_NAMESPACE    namespace Scintilla {
#define QSCI_END_SCI_NAMESPACE      };
#else
#define QSCI_SCI_NAMESPACE(name)    name
#define QSCI_BEGIN_SCI_NAMESPACE
#define QSCI_END_SCI_NAMESPACE
#endif

#endif
