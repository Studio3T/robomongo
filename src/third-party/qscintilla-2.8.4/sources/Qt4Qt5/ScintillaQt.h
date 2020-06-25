// The definition of the Qt specific subclass of ScintillaBase.
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


#ifndef SCINTILLAQT_H
#define	SCINTILLAQT_H


#include <qtimer.h>
#include <qclipboard.h>

#include <Qsci/qsciglobal.h>

#include "SciNamespace.h"

// These are needed because Scintilla class header files don't manage their own
// dependencies properly.
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string>
#include "ILexer.h"
#include "Platform.h"
#include "Scintilla.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "CellBuffer.h"
#include "CharClassify.h"
#include "RunStyles.h"
#include "CaseFolder.h"
#include "Decoration.h"
#include "Document.h"
#include "Style.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Indicator.h"
#include "ViewStyle.h"
#include "KeyMap.h"
#include "ContractionState.h"
#include "Selection.h"
#include "PositionCache.h"
#include "Editor.h"
#include "AutoComplete.h"
#include "CallTip.h"
#include "LexAccessor.h"
#include "Accessor.h"

#include "ScintillaBase.h"


QT_BEGIN_NAMESPACE
class QPaintEvent;
class QDropEvent;
class QMimeData;
QT_END_NAMESPACE

class QsciScintillaBase;
class QsciSciCallTip;
class QsciSciPopup;


// This is an internal class but it is referenced by a public class so it has
// to have a Qsci prefix rather than being put in the Scintilla namespace
// which would mean exposing the SCI_NAMESPACE mechanism).
class QsciScintillaQt : public QSCI_SCI_NAMESPACE(ScintillaBase)
{
	friend class QsciScintillaBase;
	friend class QsciSciCallTip;
	friend class QsciSciPopup;

public:
	QsciScintillaQt(QsciScintillaBase *qsb_);
	virtual ~QsciScintillaQt();

	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam,
            sptr_t lParam);

private:
	void Initialise();
	void Finalise();
	void StartDrag();
	sptr_t DefWndProc(unsigned int, uptr_t, sptr_t);
	void SetTicking(bool);
	void SetMouseCapture(bool on);
	bool HaveMouseCapture();
	void SetVerticalScrollPos();
	void SetHorizontalScrollPos();
	bool ModifyScrollBars(int nMax, int nPage);
	void ReconfigureScrollBars();
	void NotifyChange();
	void NotifyParent(QSCI_SCI_NAMESPACE(SCNotification) scn);
	void CopyToClipboard(
            const QSCI_SCI_NAMESPACE(SelectionText) &selectedText);
	void Copy();
	void Paste();
	void CreateCallTipWindow(QSCI_SCI_NAMESPACE(PRectangle) rc);
	void AddToPopUp(const char *label, int cmd = 0, bool enabled = true);
	void ClaimSelection();
	void UnclaimSelection();
	static sptr_t DirectFunction(QsciScintillaQt *sci, unsigned int iMessage,
            uptr_t wParam,sptr_t lParam);

	QMimeData *mimeSelection(
            const QSCI_SCI_NAMESPACE(SelectionText) &text) const;
	void paintEvent(QPaintEvent *e);
    void pasteFromClipboard(QClipboard::Mode mode);

	bool capturedMouse;
	QsciScintillaBase *qsb;
	QTimer qtimer;
};

#endif
