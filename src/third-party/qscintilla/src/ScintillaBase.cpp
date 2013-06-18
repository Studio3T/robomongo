// Scintilla source code edit control
/** @file ScintillaBase.cxx
 ** An enhanced subclass of Editor with calltips, autocomplete and context menu.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include <string>
#include <vector>
#include <map>

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#include "PropSetSimple.h"
#ifdef SCI_LEXER
#include "SciLexer.h"
#include "LexerModule.h"
#include "Catalogue.h"
#endif
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "AutoComplete.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "Editor.h"
#include "ScintillaBase.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

ScintillaBase::ScintillaBase() {
	displayPopupMenu = true;
	listType = 0;
	maxListWidth = 0;
}

ScintillaBase::~ScintillaBase() {
}

void ScintillaBase::Finalise() {
	Editor::Finalise();
	popup.Destroy();
}

void ScintillaBase::AddCharUTF(char *s, unsigned int len, bool treatAsDBCS) {
	bool isFillUp = ac.Active() && ac.IsFillUpChar(*s);
	if (!isFillUp) {
		Editor::AddCharUTF(s, len, treatAsDBCS);
	}
	if (ac.Active()) {
		AutoCompleteCharacterAdded(s[0]);
		// For fill ups add the character after the autocompletion has
		// triggered so containers see the key so can display a calltip.
		if (isFillUp) {
			Editor::AddCharUTF(s, len, treatAsDBCS);
		}
	}
}

void ScintillaBase::Command(int cmdId) {

	switch (cmdId) {

	case idAutoComplete:  	// Nothing to do

		break;

	case idCallTip:  	// Nothing to do

		break;

	case idcmdUndo:
		WndProc(SCI_UNDO, 0, 0);
		break;

	case idcmdRedo:
		WndProc(SCI_REDO, 0, 0);
		break;

	case idcmdCut:
		WndProc(SCI_CUT, 0, 0);
		break;

	case idcmdCopy:
		WndProc(SCI_COPY, 0, 0);
		break;

	case idcmdPaste:
		WndProc(SCI_PASTE, 0, 0);
		break;

	case idcmdDelete:
		WndProc(SCI_CLEAR, 0, 0);
		break;

	case idcmdSelectAll:
		WndProc(SCI_SELECTALL, 0, 0);
		break;
	}
}

int ScintillaBase::KeyCommand(unsigned int iMessage) {
	// Most key commands cancel autocompletion mode
	if (ac.Active()) {
		switch (iMessage) {
			// Except for these
		case SCI_LINEDOWN:
			AutoCompleteMove(1);
			return 0;
		case SCI_LINEUP:
			AutoCompleteMove(-1);
			return 0;
		case SCI_PAGEDOWN:
			AutoCompleteMove(ac.lb->GetVisibleRows());
			return 0;
		case SCI_PAGEUP:
			AutoCompleteMove(-ac.lb->GetVisibleRows());
			return 0;
		case SCI_VCHOME:
			AutoCompleteMove(-5000);
			return 0;
		case SCI_LINEEND:
			AutoCompleteMove(5000);
			return 0;
		case SCI_DELETEBACK:
			DelCharBack(true);
			AutoCompleteCharacterDeleted();
			EnsureCaretVisible();
			return 0;
		case SCI_DELETEBACKNOTLINE:
			DelCharBack(false);
			AutoCompleteCharacterDeleted();
			EnsureCaretVisible();
			return 0;
		case SCI_TAB:
			AutoCompleteCompleted();
			return 0;
		case SCI_NEWLINE:
			AutoCompleteCompleted();
			return 0;

		default:
			AutoCompleteCancel();
		}
	}

	if (ct.inCallTipMode) {
		if (
		    (iMessage != SCI_CHARLEFT) &&
		    (iMessage != SCI_CHARLEFTEXTEND) &&
		    (iMessage != SCI_CHARRIGHT) &&
		    (iMessage != SCI_CHARRIGHTEXTEND) &&
		    (iMessage != SCI_EDITTOGGLEOVERTYPE) &&
		    (iMessage != SCI_DELETEBACK) &&
		    (iMessage != SCI_DELETEBACKNOTLINE)
		) {
			ct.CallTipCancel();
		}
		if ((iMessage == SCI_DELETEBACK) || (iMessage == SCI_DELETEBACKNOTLINE)) {
			if (sel.MainCaret() <= ct.posStartCallTip) {
				ct.CallTipCancel();
			}
		}
	}
	return Editor::KeyCommand(iMessage);
}

void ScintillaBase::AutoCompleteDoubleClick(void *p) {
	ScintillaBase *sci = reinterpret_cast<ScintillaBase *>(p);
	sci->AutoCompleteCompleted();
}

void ScintillaBase::AutoCompleteStart(int lenEntered, const char *list) {
	//Platform::DebugPrintf("AutoComplete %s\n", list);
	ct.CallTipCancel();

	if (ac.chooseSingle && (listType == 0)) {
		if (list && !strchr(list, ac.GetSeparator())) {
			const char *typeSep = strchr(list, ac.GetTypesep());
			int lenInsert = typeSep ? 
				static_cast<int>(typeSep-list) : static_cast<int>(strlen(list));
			if (ac.ignoreCase) {
				SetEmptySelection(sel.MainCaret() - lenEntered);
				pdoc->DeleteChars(sel.MainCaret(), lenEntered);
				SetEmptySelection(sel.MainCaret());
				pdoc->InsertString(sel.MainCaret(), list, lenInsert);
				SetEmptySelection(sel.MainCaret() + lenInsert);
			} else {
				SetEmptySelection(sel.MainCaret());
				pdoc->InsertString(sel.MainCaret(), list + lenEntered, lenInsert - lenEntered);
				SetEmptySelection(sel.MainCaret() + lenInsert - lenEntered);
			}
			ac.Cancel();
			return;
		}
	}
	ac.Start(wMain, idAutoComplete, sel.MainCaret(), PointMainCaret(),
				lenEntered, vs.lineHeight, IsUnicodeMode(), technology);

	PRectangle rcClient = GetClientRectangle();
	Point pt = LocationFromPosition(sel.MainCaret() - lenEntered);
	PRectangle rcPopupBounds = wMain.GetMonitorRect(pt);
	if (rcPopupBounds.Height() == 0)
		rcPopupBounds = rcClient;

	int heightLB = ac.heightLBDefault;
	int widthLB = ac.widthLBDefault;
	if (pt.x >= rcClient.right - widthLB) {
		HorizontalScrollTo(xOffset + pt.x - rcClient.right + widthLB);
		Redraw();
		pt = PointMainCaret();
	}
	PRectangle rcac;
	rcac.left = pt.x - ac.lb->CaretFromEdge();
	if (pt.y >= rcPopupBounds.bottom - heightLB &&  // Wont fit below.
	        pt.y >= (rcPopupBounds.bottom + rcPopupBounds.top) / 2) { // and there is more room above.
		rcac.top = pt.y - heightLB;
		if (rcac.top < rcPopupBounds.top) {
			heightLB -= (rcPopupBounds.top - rcac.top);
			rcac.top = rcPopupBounds.top;
		}
	} else {
		rcac.top = pt.y + vs.lineHeight;
	}
	rcac.right = rcac.left + widthLB;
	rcac.bottom = Platform::Minimum(rcac.top + heightLB, rcPopupBounds.bottom);
	ac.lb->SetPositionRelative(rcac, wMain);
	ac.lb->SetFont(vs.styles[STYLE_DEFAULT].font);
	unsigned int aveCharWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;
	ac.lb->SetAverageCharWidth(aveCharWidth);
	ac.lb->SetDoubleClickAction(AutoCompleteDoubleClick, this);

	ac.SetList(list);

	// Fiddle the position of the list so it is right next to the target and wide enough for all its strings
	PRectangle rcList = ac.lb->GetDesiredRect();
	int heightAlloced = rcList.bottom - rcList.top;
	widthLB = Platform::Maximum(widthLB, rcList.right - rcList.left);
	if (maxListWidth != 0)
		widthLB = Platform::Minimum(widthLB, aveCharWidth*maxListWidth);
	// Make an allowance for large strings in list
	rcList.left = pt.x - ac.lb->CaretFromEdge();
	rcList.right = rcList.left + widthLB;
	if (((pt.y + vs.lineHeight) >= (rcPopupBounds.bottom - heightAlloced)) &&  // Wont fit below.
	        ((pt.y + vs.lineHeight / 2) >= (rcPopupBounds.bottom + rcPopupBounds.top) / 2)) { // and there is more room above.
		rcList.top = pt.y - heightAlloced;
	} else {
		rcList.top = pt.y + vs.lineHeight;
	}
	rcList.bottom = rcList.top + heightAlloced;
	ac.lb->SetPositionRelative(rcList, wMain);
	ac.Show(true);
	if (lenEntered != 0) {
		AutoCompleteMoveToCurrentWord();
	}
}

void ScintillaBase::AutoCompleteCancel() {
	if (ac.Active()) {
		SCNotification scn = {0};
		scn.nmhdr.code = SCN_AUTOCCANCELLED;
		scn.wParam = 0;
		scn.listType = 0;
		NotifyParent(scn);
	}
	ac.Cancel();
}

void ScintillaBase::AutoCompleteMove(int delta) {
	ac.Move(delta);
}

void ScintillaBase::AutoCompleteMoveToCurrentWord() {
	std::string wordCurrent = RangeText(ac.posStart - ac.startLen, sel.MainCaret());
	ac.Select(wordCurrent.c_str());
}

void ScintillaBase::AutoCompleteCharacterAdded(char ch) {
	if (ac.IsFillUpChar(ch)) {
		AutoCompleteCompleted();
	} else if (ac.IsStopChar(ch)) {
		AutoCompleteCancel();
	} else {
		AutoCompleteMoveToCurrentWord();
	}
}

void ScintillaBase::AutoCompleteCharacterDeleted() {
	if (sel.MainCaret() < ac.posStart - ac.startLen) {
		AutoCompleteCancel();
	} else if (ac.cancelAtStartPos && (sel.MainCaret() <= ac.posStart)) {
		AutoCompleteCancel();
	} else {
		AutoCompleteMoveToCurrentWord();
	}
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_AUTOCCHARDELETED;
	scn.wParam = 0;
	scn.listType = 0;
	NotifyParent(scn);
}

void ScintillaBase::AutoCompleteCompleted() {
	int item = ac.GetSelection();
	if (item == -1) {
		AutoCompleteCancel();
		return;
	}
	const std::string selected = ac.GetValue(item);

	ac.Show(false);

	SCNotification scn = {0};
	scn.nmhdr.code = listType > 0 ? SCN_USERLISTSELECTION : SCN_AUTOCSELECTION;
	scn.message = 0;
	scn.wParam = listType;
	scn.listType = listType;
	Position firstPos = ac.posStart - ac.startLen;
	scn.position = firstPos;
	scn.lParam = firstPos;
	scn.text = selected.c_str();
	NotifyParent(scn);

	if (!ac.Active())
		return;
	ac.Cancel();

	if (listType > 0)
		return;

	Position endPos = sel.MainCaret();
	if (ac.dropRestOfWord)
		endPos = pdoc->ExtendWordSelect(endPos, 1, true);
	if (endPos < firstPos)
		return;
	UndoGroup ug(pdoc);
	if (endPos != firstPos) {
		pdoc->DeleteChars(firstPos, endPos - firstPos);
	}
	SetEmptySelection(ac.posStart);
	if (item != -1) {
		pdoc->InsertCString(firstPos, selected.c_str());
		SetEmptySelection(firstPos + static_cast<int>(selected.length()));
	}
	SetLastXChosen();
}

int ScintillaBase::AutoCompleteGetCurrent() {
	if (!ac.Active())
		return -1;
	return ac.GetSelection();
}

int ScintillaBase::AutoCompleteGetCurrentText(char *buffer) {
	if (ac.Active()) {
		int item = ac.GetSelection();
		if (item != -1) {
			const std::string selected = ac.GetValue(item);
			if (buffer != NULL)
				strcpy(buffer, selected.c_str());
			return static_cast<int>(selected.length());
		}
	}
	if (buffer != NULL)
		*buffer = '\0';
	return 0;
}

void ScintillaBase::CallTipShow(Point pt, const char *defn) {
	ac.Cancel();
	// If container knows about STYLE_CALLTIP then use it in place of the
	// STYLE_DEFAULT for the face name, size and character set. Also use it
	// for the foreground and background colour.
	int ctStyle = ct.UseStyleCallTip() ? STYLE_CALLTIP : STYLE_DEFAULT;
	if (ct.UseStyleCallTip()) {
		ct.SetForeBack(vs.styles[STYLE_CALLTIP].fore, vs.styles[STYLE_CALLTIP].back);
	}
	PRectangle rc = ct.CallTipStart(sel.MainCaret(), pt,
		vs.lineHeight,
		defn,
		vs.styles[ctStyle].fontName,
		vs.styles[ctStyle].sizeZoomed,
		CodePage(),
		vs.styles[ctStyle].characterSet,
		vs.technology,
		wMain);
	// If the call-tip window would be out of the client
	// space
	PRectangle rcClient = GetClientRectangle();
	int offset = vs.lineHeight + rc.Height();
	// adjust so it displays below the text.
	if (rc.top < rcClient.top) {
		rc.top += offset;
		rc.bottom += offset;
	}
	// adjust so it displays above the text.
	if (rc.bottom > rcClient.bottom) {
		rc.top -= offset;
		rc.bottom -= offset;
	}
	// Now display the window.
	CreateCallTipWindow(rc);
	ct.wCallTip.SetPositionRelative(rc, wMain);
	ct.wCallTip.Show();
}

void ScintillaBase::CallTipClick() {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_CALLTIPCLICK;
	scn.position = ct.clickPlace;
	NotifyParent(scn);
}

void ScintillaBase::ContextMenu(Point pt) {
	if (displayPopupMenu) {
		bool writable = !WndProc(SCI_GETREADONLY, 0, 0);
		popup.CreatePopUp();
		AddToPopUp("Undo", idcmdUndo, writable && pdoc->CanUndo());
		AddToPopUp("Redo", idcmdRedo, writable && pdoc->CanRedo());
		AddToPopUp("");
		AddToPopUp("Cut", idcmdCut, writable && !sel.Empty());
		AddToPopUp("Copy", idcmdCopy, !sel.Empty());
		AddToPopUp("Paste", idcmdPaste, writable && WndProc(SCI_CANPASTE, 0, 0));
		AddToPopUp("Delete", idcmdDelete, writable && !sel.Empty());
		AddToPopUp("");
		AddToPopUp("Select All", idcmdSelectAll);
		popup.Show(pt, wMain);
	}
}

void ScintillaBase::CancelModes() {
	AutoCompleteCancel();
	ct.CallTipCancel();
	Editor::CancelModes();
}

void ScintillaBase::ButtonDown(Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) {
	CancelModes();
	Editor::ButtonDown(pt, curTime, shift, ctrl, alt);
}

#ifdef SCI_LEXER

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class LexState : public LexInterface {
	const LexerModule *lexCurrent;
	void SetLexerModule(const LexerModule *lex);
	PropSetSimple props;
public:
	int lexLanguage;

	LexState(Document *pdoc_);
	virtual ~LexState();
	void SetLexer(uptr_t wParam);
	void SetLexerLanguage(const char *languageName);
	const char *DescribeWordListSets();
	void SetWordList(int n, const char *wl);
	int GetStyleBitsNeeded() const;
	const char *GetName() const;
	void *PrivateCall(int operation, void *pointer);
	const char *PropertyNames();
	int PropertyType(const char *name);
	const char *DescribeProperty(const char *name);
	void PropSet(const char *key, const char *val);
	const char *PropGet(const char *key) const;
	int PropGetInt(const char *key, int defaultValue=0) const;
	int PropGetExpanded(const char *key, char *result) const;
};

#ifdef SCI_NAMESPACE
}
#endif

LexState::LexState(Document *pdoc_) : LexInterface(pdoc_) {
	lexCurrent = 0;
	performingStyle = false;
	lexLanguage = SCLEX_CONTAINER;
}

LexState::~LexState() {
	if (instance) {
		instance->Release();
		instance = 0;
	}
}

LexState *ScintillaBase::DocumentLexState() {
	if (!pdoc->pli) {
		pdoc->pli = new LexState(pdoc);
	}
	return static_cast<LexState *>(pdoc->pli);
}

void LexState::SetLexerModule(const LexerModule *lex) {
	if (lex != lexCurrent) {
		if (instance) {
			instance->Release();
			instance = 0;
		}
		lexCurrent = lex;
		if (lexCurrent)
			instance = lexCurrent->Create();
		pdoc->LexerChanged();
	}
}

void LexState::SetLexer(uptr_t wParam) {
	lexLanguage = wParam;
	if (lexLanguage == SCLEX_CONTAINER) {
		SetLexerModule(0);
	} else {
		const LexerModule *lex = Catalogue::Find(lexLanguage);
		if (!lex)
			lex = Catalogue::Find(SCLEX_NULL);
		SetLexerModule(lex);
	}
}

void LexState::SetLexerLanguage(const char *languageName) {
	const LexerModule *lex = Catalogue::Find(languageName);
	if (!lex)
		lex = Catalogue::Find(SCLEX_NULL);
	if (lex)
		lexLanguage = lex->GetLanguage();
	SetLexerModule(lex);
}

const char *LexState::DescribeWordListSets() {
	if (instance) {
		return instance->DescribeWordListSets();
	} else {
		return 0;
	}
}

void LexState::SetWordList(int n, const char *wl) {
	if (instance) {
		int firstModification = instance->WordListSet(n, wl);
		if (firstModification >= 0) {
			pdoc->ModifiedAt(firstModification);
		}
	}
}

int LexState::GetStyleBitsNeeded() const {
	return lexCurrent ? lexCurrent->GetStyleBitsNeeded() : 5;
}

const char *LexState::GetName() const {
	return lexCurrent ? lexCurrent->languageName : "";
}

void *LexState::PrivateCall(int operation, void *pointer) {
	if (pdoc && instance) {
		return instance->PrivateCall(operation, pointer);
	} else {
		return 0;
	}
}

const char *LexState::PropertyNames() {
	if (instance) {
		return instance->PropertyNames();
	} else {
		return 0;
	}
}

int LexState::PropertyType(const char *name) {
	if (instance) {
		return instance->PropertyType(name);
	} else {
		return SC_TYPE_BOOLEAN;
	}
}

const char *LexState::DescribeProperty(const char *name) {
	if (instance) {
		return instance->DescribeProperty(name);
	} else {
		return 0;
	}
}

void LexState::PropSet(const char *key, const char *val) {
	props.Set(key, val);
	if (instance) {
		int firstModification = instance->PropertySet(key, val);
		if (firstModification >= 0) {
			pdoc->ModifiedAt(firstModification);
		}
	}
}

const char *LexState::PropGet(const char *key) const {
	return props.Get(key);
}

int LexState::PropGetInt(const char *key, int defaultValue) const {
	return props.GetInt(key, defaultValue);
}

int LexState::PropGetExpanded(const char *key, char *result) const {
	return props.GetExpanded(key, result);
}

#endif

void ScintillaBase::NotifyStyleToNeeded(int endStyleNeeded) {
#ifdef SCI_LEXER
	if (DocumentLexState()->lexLanguage != SCLEX_CONTAINER) {
		int lineEndStyled = pdoc->LineFromPosition(pdoc->GetEndStyled());
		int endStyled = pdoc->LineStart(lineEndStyled);
		DocumentLexState()->Colourise(endStyled, endStyleNeeded);
		return;
	}
#endif
	Editor::NotifyStyleToNeeded(endStyleNeeded);
}

void ScintillaBase::NotifyLexerChanged(Document *, void *) {
#ifdef SCI_LEXER
	int bits = DocumentLexState()->GetStyleBitsNeeded();
	vs.EnsureStyle((1 << bits) - 1);
#endif
}

sptr_t ScintillaBase::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {
	case SCI_AUTOCSHOW:
		listType = 0;
		AutoCompleteStart(wParam, reinterpret_cast<const char *>(lParam));
		break;

	case SCI_AUTOCCANCEL:
		ac.Cancel();
		break;

	case SCI_AUTOCACTIVE:
		return ac.Active();

	case SCI_AUTOCPOSSTART:
		return ac.posStart;

	case SCI_AUTOCCOMPLETE:
		AutoCompleteCompleted();
		break;

	case SCI_AUTOCSETSEPARATOR:
		ac.SetSeparator(static_cast<char>(wParam));
		break;

	case SCI_AUTOCGETSEPARATOR:
		return ac.GetSeparator();

	case SCI_AUTOCSTOPS:
		ac.SetStopChars(reinterpret_cast<char *>(lParam));
		break;

	case SCI_AUTOCSELECT:
		ac.Select(reinterpret_cast<char *>(lParam));
		break;

	case SCI_AUTOCGETCURRENT:
		return AutoCompleteGetCurrent();

	case SCI_AUTOCGETCURRENTTEXT:
		return AutoCompleteGetCurrentText(reinterpret_cast<char *>(lParam));

	case SCI_AUTOCSETCANCELATSTART:
		ac.cancelAtStartPos = wParam != 0;
		break;

	case SCI_AUTOCGETCANCELATSTART:
		return ac.cancelAtStartPos;

	case SCI_AUTOCSETFILLUPS:
		ac.SetFillUpChars(reinterpret_cast<char *>(lParam));
		break;

	case SCI_AUTOCSETCHOOSESINGLE:
		ac.chooseSingle = wParam != 0;
		break;

	case SCI_AUTOCGETCHOOSESINGLE:
		return ac.chooseSingle;

	case SCI_AUTOCSETIGNORECASE:
		ac.ignoreCase = wParam != 0;
		break;

	case SCI_AUTOCGETIGNORECASE:
		return ac.ignoreCase;

	case SCI_AUTOCSETCASEINSENSITIVEBEHAVIOUR:
		ac.ignoreCaseBehaviour = wParam;
		break;

	case SCI_AUTOCGETCASEINSENSITIVEBEHAVIOUR:
		return ac.ignoreCaseBehaviour;

	case SCI_USERLISTSHOW:
		listType = wParam;
		AutoCompleteStart(0, reinterpret_cast<const char *>(lParam));
		break;

	case SCI_AUTOCSETAUTOHIDE:
		ac.autoHide = wParam != 0;
		break;

	case SCI_AUTOCGETAUTOHIDE:
		return ac.autoHide;

	case SCI_AUTOCSETDROPRESTOFWORD:
		ac.dropRestOfWord = wParam != 0;
		break;

	case SCI_AUTOCGETDROPRESTOFWORD:
		return ac.dropRestOfWord;

	case SCI_AUTOCSETMAXHEIGHT:
		ac.lb->SetVisibleRows(wParam);
		break;

	case SCI_AUTOCGETMAXHEIGHT:
		return ac.lb->GetVisibleRows();

	case SCI_AUTOCSETMAXWIDTH:
		maxListWidth = wParam;
		break;

	case SCI_AUTOCGETMAXWIDTH:
		return maxListWidth;

	case SCI_REGISTERIMAGE:
		ac.lb->RegisterImage(wParam, reinterpret_cast<const char *>(lParam));
		break;

	case SCI_REGISTERRGBAIMAGE:
		ac.lb->RegisterRGBAImage(wParam, sizeRGBAImage.x, sizeRGBAImage.y, reinterpret_cast<unsigned char *>(lParam));
		break;

	case SCI_CLEARREGISTEREDIMAGES:
		ac.lb->ClearRegisteredImages();
		break;

	case SCI_AUTOCSETTYPESEPARATOR:
		ac.SetTypesep(static_cast<char>(wParam));
		break;

	case SCI_AUTOCGETTYPESEPARATOR:
		return ac.GetTypesep();

	case SCI_CALLTIPSHOW:
		CallTipShow(LocationFromPosition(wParam),
			reinterpret_cast<const char *>(lParam));
		break;

	case SCI_CALLTIPCANCEL:
		ct.CallTipCancel();
		break;

	case SCI_CALLTIPACTIVE:
		return ct.inCallTipMode;

	case SCI_CALLTIPPOSSTART:
		return ct.posStartCallTip;

	case SCI_CALLTIPSETHLT:
		ct.SetHighlight(wParam, lParam);
		break;

	case SCI_CALLTIPSETBACK:
		ct.colourBG = ColourDesired(wParam);
		vs.styles[STYLE_CALLTIP].back = ct.colourBG;
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPSETFORE:
		ct.colourUnSel = ColourDesired(wParam);
		vs.styles[STYLE_CALLTIP].fore = ct.colourUnSel;
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPSETFOREHLT:
		ct.colourSel = ColourDesired(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPUSESTYLE:
		ct.SetTabSize((int)wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPSETPOSITION:
		ct.SetPosition(wParam != 0);
		InvalidateStyleRedraw();
		break;

	case SCI_USEPOPUP:
		displayPopupMenu = wParam != 0;
		break;

#ifdef SCI_LEXER
	case SCI_SETLEXER:
		DocumentLexState()->SetLexer(wParam);
		break;

	case SCI_GETLEXER:
		return DocumentLexState()->lexLanguage;

	case SCI_COLOURISE:
		if (DocumentLexState()->lexLanguage == SCLEX_CONTAINER) {
			pdoc->ModifiedAt(wParam);
			NotifyStyleToNeeded((lParam == -1) ? pdoc->Length() : lParam);
		} else {
			DocumentLexState()->Colourise(wParam, lParam);
		}
		Redraw();
		break;

	case SCI_SETPROPERTY:
		DocumentLexState()->PropSet(reinterpret_cast<const char *>(wParam),
		          reinterpret_cast<const char *>(lParam));
		break;

	case SCI_GETPROPERTY:
		return StringResult(lParam, DocumentLexState()->PropGet(reinterpret_cast<const char *>(wParam)));

	case SCI_GETPROPERTYEXPANDED:
		return DocumentLexState()->PropGetExpanded(reinterpret_cast<const char *>(wParam),
			reinterpret_cast<char *>(lParam));

	case SCI_GETPROPERTYINT:
		return DocumentLexState()->PropGetInt(reinterpret_cast<const char *>(wParam), lParam);

	case SCI_SETKEYWORDS:
		DocumentLexState()->SetWordList(wParam, reinterpret_cast<const char *>(lParam));
		break;

	case SCI_SETLEXERLANGUAGE:
		DocumentLexState()->SetLexerLanguage(reinterpret_cast<const char *>(lParam));
		break;

	case SCI_GETLEXERLANGUAGE:
		return StringResult(lParam, DocumentLexState()->GetName());

	case SCI_PRIVATELEXERCALL:
		return reinterpret_cast<sptr_t>(
			DocumentLexState()->PrivateCall(wParam, reinterpret_cast<void *>(lParam)));

	case SCI_GETSTYLEBITSNEEDED:
		return DocumentLexState()->GetStyleBitsNeeded();

	case SCI_PROPERTYNAMES:
		return StringResult(lParam, DocumentLexState()->PropertyNames());

	case SCI_PROPERTYTYPE:
		return DocumentLexState()->PropertyType(reinterpret_cast<const char *>(wParam));

	case SCI_DESCRIBEPROPERTY:
		return StringResult(lParam, DocumentLexState()->DescribeProperty(reinterpret_cast<const char *>(wParam)));

	case SCI_DESCRIBEKEYWORDSETS:
		return StringResult(lParam, DocumentLexState()->DescribeWordListSets());

#endif

	default:
		return Editor::WndProc(iMessage, wParam, lParam);
	}
	return 0l;
}
