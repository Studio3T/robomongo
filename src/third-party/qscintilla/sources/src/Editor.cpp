// Scintilla source code edit control
/** @file Editor.cxx
 ** Main code for the edit control.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#include "StringCopy.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "PerLine.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

/*
	return whether this modification represents an operation that
	may reasonably be deferred (not done now OR [possibly] at all)
*/
static bool CanDeferToLastStep(const DocModification &mh) {
	if (mh.modificationType & (SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE))
		return true;	// CAN skip
	if (!(mh.modificationType & (SC_PERFORMED_UNDO | SC_PERFORMED_REDO)))
		return false;	// MUST do
	if (mh.modificationType & SC_MULTISTEPUNDOREDO)
		return true;	// CAN skip
	return false;		// PRESUMABLY must do
}

static bool CanEliminate(const DocModification &mh) {
	return
	    (mh.modificationType & (SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE)) != 0;
}

/*
	return whether this modification represents the FINAL step
	in a [possibly lengthy] multi-step Undo/Redo sequence
*/
static bool IsLastStep(const DocModification &mh) {
	return
	    (mh.modificationType & (SC_PERFORMED_UNDO | SC_PERFORMED_REDO)) != 0
	    && (mh.modificationType & SC_MULTISTEPUNDOREDO) != 0
	    && (mh.modificationType & SC_LASTSTEPINUNDOREDO) != 0
	    && (mh.modificationType & SC_MULTILINEUNDOREDO) != 0;
}

Timer::Timer() :
		ticking(false), ticksToWait(0), tickerID(0) {}

Idler::Idler() :
		state(false), idlerID(0) {}

static inline bool IsAllSpacesOrTabs(const char *s, unsigned int len) {
	for (unsigned int i = 0; i < len; i++) {
		// This is safe because IsSpaceOrTab() will return false for null terminators
		if (!IsSpaceOrTab(s[i]))
			return false;
	}
	return true;
}

Editor::Editor() {
	ctrlID = 0;

	stylesValid = false;
	technology = SC_TECHNOLOGY_DEFAULT;
	scaleRGBAImage = 100.0f;

	cursorMode = SC_CURSORNORMAL;

	hasFocus = false;
	errorStatus = 0;
	mouseDownCaptures = true;

	lastClickTime = 0;
	doubleClickCloseThreshold = Point(3, 3);
	dwellDelay = SC_TIME_FOREVER;
	ticksToDwell = SC_TIME_FOREVER;
	dwelling = false;
	ptMouseLast.x = 0;
	ptMouseLast.y = 0;
	inDragDrop = ddNone;
	dropWentOutside = false;
	posDrop = SelectionPosition(invalidPosition);
	hotSpotClickPos = INVALID_POSITION;
	selectionType = selChar;

	lastXChosen = 0;
	lineAnchorPos = 0;
	originalAnchorPos = 0;
	wordSelectAnchorStartPos = 0;
	wordSelectAnchorEndPos = 0;
	wordSelectInitialCaretPos = -1;

	caretXPolicy = CARET_SLOP | CARET_EVEN;
	caretXSlop = 50;

	caretYPolicy = CARET_EVEN;
	caretYSlop = 0;

	visiblePolicy = 0;
	visibleSlop = 0;

	searchAnchor = 0;

	xCaretMargin = 50;
	horizontalScrollBarVisible = true;
	scrollWidth = 2000;
	verticalScrollBarVisible = true;
	endAtLastLine = true;
	caretSticky = SC_CARETSTICKY_OFF;
	marginOptions = SC_MARGINOPTION_NONE;
	mouseSelectionRectangularSwitch = false;
	multipleSelection = false;
	additionalSelectionTyping = false;
	multiPasteMode = SC_MULTIPASTE_ONCE;
	virtualSpaceOptions = SCVS_NONE;

	targetStart = 0;
	targetEnd = 0;
	searchFlags = 0;

	topLine = 0;
	posTopLine = 0;

	lengthForEncode = -1;

	needUpdateUI = 0;
	ContainerNeedsUpdate(SC_UPDATE_CONTENT);

	paintState = notPainting;
	paintAbandonedByStyling = false;
	paintingAllText = false;
	willRedrawAll = false;

	modEventMask = SC_MODEVENTMASKALL;

	pdoc->AddWatcher(this, 0);

	recordingMacro = false;
	foldAutomatic = 0;

	convertPastes = true;

	SetRepresentations();
}

Editor::~Editor() {
	pdoc->RemoveWatcher(this, 0);
	DropGraphics(true);
}

void Editor::Finalise() {
	SetIdle(false);
	CancelModes();
}

void Editor::SetRepresentations() {
	reprs.Clear();

	// C0 control set
	const char *reps[] = {
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		"BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US"
	};
	for (size_t j=0; j < ELEMENTS(reps); j++) {
		char c[2] = { static_cast<char>(j), 0 };
		reprs.SetRepresentation(c, reps[j]);
	}

	// C1 control set
	// As well as Unicode mode, ISO-8859-1 should use these
	if (IsUnicodeMode()) {
		const char *repsC1[] = {
			"PAD", "HOP", "BPH", "NBH", "IND", "NEL", "SSA", "ESA",
			"HTS", "HTJ", "VTS", "PLD", "PLU", "RI", "SS2", "SS3",
			"DCS", "PU1", "PU2", "STS", "CCH", "MW", "SPA", "EPA",
			"SOS", "SGCI", "SCI", "CSI", "ST", "OSC", "PM", "APC"
		};
		for (size_t j=0; j < ELEMENTS(repsC1); j++) {
			char c1[3] = { '\xc2',  static_cast<char>(0x80+j), 0 };
			reprs.SetRepresentation(c1, repsC1[j]);
		}
		reprs.SetRepresentation("\xe2\x80\xa8", "LS");
		reprs.SetRepresentation("\xe2\x80\xa9", "PS");
	}

	// UTF-8 invalid bytes
	if (IsUnicodeMode()) {
		for (int k=0x80; k < 0x100; k++) {
			char hiByte[2] = {  static_cast<char>(k), 0 };
			char hexits[4];
			sprintf(hexits, "x%2X", k);
			reprs.SetRepresentation(hiByte, hexits);
		}
	}
}

void Editor::DropGraphics(bool freeObjects) {
	marginView.DropGraphics(freeObjects);
	view.DropGraphics(freeObjects);
}

void Editor::AllocateGraphics() {
	marginView.AllocateGraphics(vs);
	view.AllocateGraphics(vs);
}

void Editor::InvalidateStyleData() {
	stylesValid = false;
	vs.technology = technology;
	DropGraphics(false);
	AllocateGraphics();
	view.llc.Invalidate(LineLayout::llInvalid);
	view.posCache.Clear();
}

void Editor::InvalidateStyleRedraw() {
	NeedWrapping();
	InvalidateStyleData();
	Redraw();
}

void Editor::RefreshStyleData() {
	if (!stylesValid) {
		stylesValid = true;
		AutoSurface surface(this);
		if (surface) {
			vs.Refresh(*surface, pdoc->tabInChars);
		}
		SetScrollBars();
		SetRectangularRange();
	}
}

Point Editor::GetVisibleOriginInMain() const {
	return Point(0,0);
}

Point Editor::DocumentPointFromView(Point ptView) const {
	Point ptDocument = ptView;
	if (wMargin.GetID()) {
		Point ptOrigin = GetVisibleOriginInMain();
		ptDocument.x += ptOrigin.x;
		ptDocument.y += ptOrigin.y;
	} else {
		ptDocument.x += xOffset;
		ptDocument.y += topLine * vs.lineHeight;
	}
	return ptDocument;
}

int Editor::TopLineOfMain() const {
	if (wMargin.GetID())
		return 0;
	else
		return topLine;
}

PRectangle Editor::GetClientRectangle() const {
	Window &win = const_cast<Window &>(wMain);
	return win.GetClientPosition();
}

PRectangle Editor::GetClientDrawingRectangle() {
	return GetClientRectangle();
}

PRectangle Editor::GetTextRectangle() const {
	PRectangle rc = GetClientRectangle();
	rc.left += vs.textStart;
	rc.right -= vs.rightMarginWidth;
	return rc;
}

int Editor::LinesOnScreen() const {
	PRectangle rcClient = GetClientRectangle();
	int htClient = static_cast<int>(rcClient.bottom - rcClient.top);
	//Platform::DebugPrintf("lines on screen = %d\n", htClient / lineHeight + 1);
	return htClient / vs.lineHeight;
}

int Editor::LinesToScroll() const {
	int retVal = LinesOnScreen() - 1;
	if (retVal < 1)
		return 1;
	else
		return retVal;
}

int Editor::MaxScrollPos() const {
	//Platform::DebugPrintf("Lines %d screen = %d maxScroll = %d\n",
	//LinesTotal(), LinesOnScreen(), LinesTotal() - LinesOnScreen() + 1);
	int retVal = cs.LinesDisplayed();
	if (endAtLastLine) {
		retVal -= LinesOnScreen();
	} else {
		retVal--;
	}
	if (retVal < 0) {
		return 0;
	} else {
		return retVal;
	}
}

SelectionPosition Editor::ClampPositionIntoDocument(SelectionPosition sp) const {
	if (sp.Position() < 0) {
		return SelectionPosition(0);
	} else if (sp.Position() > pdoc->Length()) {
		return SelectionPosition(pdoc->Length());
	} else {
		// If not at end of line then set offset to 0
		if (!pdoc->IsLineEndPosition(sp.Position()))
			sp.SetVirtualSpace(0);
		return sp;
	}
}

Point Editor::LocationFromPosition(SelectionPosition pos) {
	RefreshStyleData();
	AutoSurface surface(this);
	return view.LocationFromPosition(surface, *this, pos, topLine, vs);
}

Point Editor::LocationFromPosition(int pos) {
	return LocationFromPosition(SelectionPosition(pos));
}

int Editor::XFromPosition(int pos) {
	Point pt = LocationFromPosition(pos);
	return static_cast<int>(pt.x) - vs.textStart + xOffset;
}

int Editor::XFromPosition(SelectionPosition sp) {
	Point pt = LocationFromPosition(sp);
	return static_cast<int>(pt.x) - vs.textStart + xOffset;
}

SelectionPosition Editor::SPositionFromLocation(Point pt, bool canReturnInvalid, bool charPosition, bool virtualSpace) {
	RefreshStyleData();
	AutoSurface surface(this);

	if (canReturnInvalid) {
		PRectangle rcClient = GetTextRectangle();
		// May be in scroll view coordinates so translate back to main view
		Point ptOrigin = GetVisibleOriginInMain();
		rcClient.Move(-ptOrigin.x, -ptOrigin.y);
		if (!rcClient.Contains(pt))
			return SelectionPosition(INVALID_POSITION);
		if (pt.x < vs.textStart)
			return SelectionPosition(INVALID_POSITION);
		if (pt.y < 0)
			return SelectionPosition(INVALID_POSITION);
	}
	pt = DocumentPointFromView(pt);
	return view.SPositionFromLocation(surface, *this, pt, canReturnInvalid, charPosition, virtualSpace, vs);
}

int Editor::PositionFromLocation(Point pt, bool canReturnInvalid, bool charPosition) {
	return SPositionFromLocation(pt, canReturnInvalid, charPosition, false).Position();
}

/**
* Find the document position corresponding to an x coordinate on a particular document line.
* Ensure is between whole characters when document is in multi-byte or UTF-8 mode.
* This method is used for rectangular selections and does not work on wrapped lines.
*/
SelectionPosition Editor::SPositionFromLineX(int lineDoc, int x) {
	RefreshStyleData();
	if (lineDoc >= pdoc->LinesTotal())
		return SelectionPosition(pdoc->Length());
	//Platform::DebugPrintf("Position of (%d,%d) line = %d top=%d\n", pt.x, pt.y, line, topLine);
	AutoSurface surface(this);
	return view.SPositionFromLineX(surface, *this, lineDoc, x, vs);
}

int Editor::PositionFromLineX(int lineDoc, int x) {
	return SPositionFromLineX(lineDoc, x).Position();
}

int Editor::LineFromLocation(Point pt) const {
	return cs.DocFromDisplay(static_cast<int>(pt.y) / vs.lineHeight + topLine);
}

void Editor::SetTopLine(int topLineNew) {
	if ((topLine != topLineNew) && (topLineNew >= 0)) {
		topLine = topLineNew;
		ContainerNeedsUpdate(SC_UPDATE_V_SCROLL);
	}
	posTopLine = pdoc->LineStart(cs.DocFromDisplay(topLine));
}

/**
 * If painting then abandon the painting because a wider redraw is needed.
 * @return true if calling code should stop drawing.
 */
bool Editor::AbandonPaint() {
	if ((paintState == painting) && !paintingAllText) {
		paintState = paintAbandoned;
	}
	return paintState == paintAbandoned;
}

void Editor::RedrawRect(PRectangle rc) {
	//Platform::DebugPrintf("Redraw %0d,%0d - %0d,%0d\n", rc.left, rc.top, rc.right, rc.bottom);

	// Clip the redraw rectangle into the client area
	PRectangle rcClient = GetClientRectangle();
	if (rc.top < rcClient.top)
		rc.top = rcClient.top;
	if (rc.bottom > rcClient.bottom)
		rc.bottom = rcClient.bottom;
	if (rc.left < rcClient.left)
		rc.left = rcClient.left;
	if (rc.right > rcClient.right)
		rc.right = rcClient.right;

	if ((rc.bottom > rc.top) && (rc.right > rc.left)) {
		wMain.InvalidateRectangle(rc);
	}
}

void Editor::DiscardOverdraw() {
	// Overridden on platforms that may draw outside visible area.
}

void Editor::Redraw() {
	//Platform::DebugPrintf("Redraw all\n");
	PRectangle rcClient = GetClientRectangle();
	wMain.InvalidateRectangle(rcClient);
	if (wMargin.GetID())
		wMargin.InvalidateAll();
	//wMain.InvalidateAll();
}

void Editor::RedrawSelMargin(int line, bool allAfter) {
	bool abandonDraw = false;
	if (!wMargin.GetID())	// Margin in main window so may need to abandon and retry
		abandonDraw = AbandonPaint();
	if (!abandonDraw) {
		if (vs.maskInLine) {
			Redraw();
		} else {
			PRectangle rcSelMargin = GetClientRectangle();
			rcSelMargin.right = rcSelMargin.left + vs.fixedColumnWidth;
			if (line != -1) {
				PRectangle rcLine = RectangleFromRange(Range(pdoc->LineStart(line)), 0);

				// Inflate line rectangle if there are image markers with height larger than line height
				if (vs.largestMarkerHeight > vs.lineHeight) {
					int delta = (vs.largestMarkerHeight - vs.lineHeight + 1) / 2;
					rcLine.top -= delta;
					rcLine.bottom += delta;
					if (rcLine.top < rcSelMargin.top)
						rcLine.top = rcSelMargin.top;
					if (rcLine.bottom > rcSelMargin.bottom)
						rcLine.bottom = rcSelMargin.bottom;
				}

				rcSelMargin.top = rcLine.top;
				if (!allAfter)
					rcSelMargin.bottom = rcLine.bottom;
				if (rcSelMargin.Empty())
					return;
			}
			if (wMargin.GetID()) {
				Point ptOrigin = GetVisibleOriginInMain();
				rcSelMargin.Move(-ptOrigin.x, -ptOrigin.y);
				wMargin.InvalidateRectangle(rcSelMargin);
			} else {
				wMain.InvalidateRectangle(rcSelMargin);
			}
		}
	}
}

PRectangle Editor::RectangleFromRange(Range r, int overlap) {
	const int minLine = cs.DisplayFromDoc(pdoc->LineFromPosition(r.First()));
	const int maxLine = cs.DisplayLastFromDoc(pdoc->LineFromPosition(r.Last()));
	const PRectangle rcClientDrawing = GetClientDrawingRectangle();
	PRectangle rc;
	const int leftTextOverlap = ((xOffset == 0) && (vs.leftMarginWidth > 0)) ? 1 : 0;
	rc.left = static_cast<XYPOSITION>(vs.textStart - leftTextOverlap);
	rc.top = static_cast<XYPOSITION>((minLine - TopLineOfMain()) * vs.lineHeight - overlap);
	if (rc.top < rcClientDrawing.top)
		rc.top = rcClientDrawing.top;
	// Extend to right of prepared area if any to prevent artifacts from caret line highlight
	rc.right = rcClientDrawing.right;
	rc.bottom = static_cast<XYPOSITION>((maxLine - TopLineOfMain() + 1) * vs.lineHeight + overlap);

	return rc;
}

void Editor::InvalidateRange(int start, int end) {
	RedrawRect(RectangleFromRange(Range(start, end), view.LinesOverlap() ? vs.lineOverlap : 0));
}

int Editor::CurrentPosition() const {
	return sel.MainCaret();
}

bool Editor::SelectionEmpty() const {
	return sel.Empty();
}

SelectionPosition Editor::SelectionStart() {
	return sel.RangeMain().Start();
}

SelectionPosition Editor::SelectionEnd() {
	return sel.RangeMain().End();
}

void Editor::SetRectangularRange() {
	if (sel.IsRectangular()) {
		int xAnchor = XFromPosition(sel.Rectangular().anchor);
		int xCaret = XFromPosition(sel.Rectangular().caret);
		if (sel.selType == Selection::selThin) {
			xCaret = xAnchor;
		}
		int lineAnchorRect = pdoc->LineFromPosition(sel.Rectangular().anchor.Position());
		int lineCaret = pdoc->LineFromPosition(sel.Rectangular().caret.Position());
		int increment = (lineCaret > lineAnchorRect) ? 1 : -1;
		for (int line=lineAnchorRect; line != lineCaret+increment; line += increment) {
			SelectionRange range(SPositionFromLineX(line, xCaret), SPositionFromLineX(line, xAnchor));
			if ((virtualSpaceOptions & SCVS_RECTANGULARSELECTION) == 0)
				range.ClearVirtualSpace();
			if (line == lineAnchorRect)
				sel.SetSelection(range);
			else
				sel.AddSelectionWithoutTrim(range);
		}
	}
}

void Editor::ThinRectangularRange() {
	if (sel.IsRectangular()) {
		sel.selType = Selection::selThin;
		if (sel.Rectangular().caret < sel.Rectangular().anchor) {
			sel.Rectangular() = SelectionRange(sel.Range(sel.Count()-1).caret, sel.Range(0).anchor);
		} else {
			sel.Rectangular() = SelectionRange(sel.Range(sel.Count()-1).anchor, sel.Range(0).caret);
		}
		SetRectangularRange();
	}
}

void Editor::InvalidateSelection(SelectionRange newMain, bool invalidateWholeSelection) {
	if (sel.Count() > 1 || !(sel.RangeMain().anchor == newMain.anchor) || sel.IsRectangular()) {
		invalidateWholeSelection = true;
	}
	int firstAffected = Platform::Minimum(sel.RangeMain().Start().Position(), newMain.Start().Position());
	// +1 for lastAffected ensures caret repainted
	int lastAffected = Platform::Maximum(newMain.caret.Position()+1, newMain.anchor.Position());
	lastAffected = Platform::Maximum(lastAffected, sel.RangeMain().End().Position());
	if (invalidateWholeSelection) {
		for (size_t r=0; r<sel.Count(); r++) {
			firstAffected = Platform::Minimum(firstAffected, sel.Range(r).caret.Position());
			firstAffected = Platform::Minimum(firstAffected, sel.Range(r).anchor.Position());
			lastAffected = Platform::Maximum(lastAffected, sel.Range(r).caret.Position()+1);
			lastAffected = Platform::Maximum(lastAffected, sel.Range(r).anchor.Position());
		}
	}
	ContainerNeedsUpdate(SC_UPDATE_SELECTION);
	InvalidateRange(firstAffected, lastAffected);
}

void Editor::SetSelection(SelectionPosition currentPos_, SelectionPosition anchor_) {
	currentPos_ = ClampPositionIntoDocument(currentPos_);
	anchor_ = ClampPositionIntoDocument(anchor_);
	int currentLine = pdoc->LineFromPosition(currentPos_.Position());
	/* For Line selection - ensure the anchor and caret are always
	   at the beginning and end of the region lines. */
	if (sel.selType == Selection::selLines) {
		if (currentPos_ > anchor_) {
			anchor_ = SelectionPosition(pdoc->LineStart(pdoc->LineFromPosition(anchor_.Position())));
			currentPos_ = SelectionPosition(pdoc->LineEnd(pdoc->LineFromPosition(currentPos_.Position())));
		} else {
			currentPos_ = SelectionPosition(pdoc->LineStart(pdoc->LineFromPosition(currentPos_.Position())));
			anchor_ = SelectionPosition(pdoc->LineEnd(pdoc->LineFromPosition(anchor_.Position())));
		}
	}
	SelectionRange rangeNew(currentPos_, anchor_);
	if (sel.Count() > 1 || !(sel.RangeMain() == rangeNew)) {
		InvalidateSelection(rangeNew);
	}
	sel.RangeMain() = rangeNew;
	SetRectangularRange();
	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());

	if (marginView.highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkNeeded::workUpdateUI);
}

void Editor::SetSelection(int currentPos_, int anchor_) {
	SetSelection(SelectionPosition(currentPos_), SelectionPosition(anchor_));
}

// Just move the caret on the main selection
void Editor::SetSelection(SelectionPosition currentPos_) {
	currentPos_ = ClampPositionIntoDocument(currentPos_);
	int currentLine = pdoc->LineFromPosition(currentPos_.Position());
	if (sel.Count() > 1 || !(sel.RangeMain().caret == currentPos_)) {
		InvalidateSelection(SelectionRange(currentPos_));
	}
	if (sel.IsRectangular()) {
		sel.Rectangular() =
			SelectionRange(SelectionPosition(currentPos_), sel.Rectangular().anchor);
		SetRectangularRange();
	} else {
		sel.RangeMain() =
			SelectionRange(SelectionPosition(currentPos_), sel.RangeMain().anchor);
	}
	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());

	if (marginView.highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkNeeded::workUpdateUI);
}

void Editor::SetSelection(int currentPos_) {
	SetSelection(SelectionPosition(currentPos_));
}

void Editor::SetEmptySelection(SelectionPosition currentPos_) {
	int currentLine = pdoc->LineFromPosition(currentPos_.Position());
	SelectionRange rangeNew(ClampPositionIntoDocument(currentPos_));
	if (sel.Count() > 1 || !(sel.RangeMain() == rangeNew)) {
		InvalidateSelection(rangeNew);
	}
	sel.Clear();
	sel.RangeMain() = rangeNew;
	SetRectangularRange();
	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());

	if (marginView.highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	QueueIdleWork(WorkNeeded::workUpdateUI);
}

void Editor::SetEmptySelection(int currentPos_) {
	SetEmptySelection(SelectionPosition(currentPos_));
}

bool Editor::RangeContainsProtected(int start, int end) const {
	if (vs.ProtectionActive()) {
		if (start > end) {
			int t = start;
			start = end;
			end = t;
		}
		for (int pos = start; pos < end; pos++) {
			if (vs.styles[pdoc->StyleAt(pos)].IsProtected())
				return true;
		}
	}
	return false;
}

bool Editor::SelectionContainsProtected() {
	for (size_t r=0; r<sel.Count(); r++) {
		if (RangeContainsProtected(sel.Range(r).Start().Position(),
			sel.Range(r).End().Position())) {
			return true;
		}
	}
	return false;
}

/**
 * Asks document to find a good position and then moves out of any invisible positions.
 */
int Editor::MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd) const {
	return MovePositionOutsideChar(SelectionPosition(pos), moveDir, checkLineEnd).Position();
}

SelectionPosition Editor::MovePositionOutsideChar(SelectionPosition pos, int moveDir, bool checkLineEnd) const {
	int posMoved = pdoc->MovePositionOutsideChar(pos.Position(), moveDir, checkLineEnd);
	if (posMoved != pos.Position())
		pos.SetPosition(posMoved);
	if (vs.ProtectionActive()) {
		if (moveDir > 0) {
			if ((pos.Position() > 0) && vs.styles[pdoc->StyleAt(pos.Position() - 1)].IsProtected()) {
				while ((pos.Position() < pdoc->Length()) &&
				        (vs.styles[pdoc->StyleAt(pos.Position())].IsProtected()))
					pos.Add(1);
			}
		} else if (moveDir < 0) {
			if (vs.styles[pdoc->StyleAt(pos.Position())].IsProtected()) {
				while ((pos.Position() > 0) &&
				        (vs.styles[pdoc->StyleAt(pos.Position() - 1)].IsProtected()))
					pos.Add(-1);
			}
		}
	}
	return pos;
}

int Editor::MovePositionTo(SelectionPosition newPos, Selection::selTypes selt, bool ensureVisible) {
	bool simpleCaret = (sel.Count() == 1) && sel.Empty();
	SelectionPosition spCaret = sel.Last();

	int delta = newPos.Position() - sel.MainCaret();
	newPos = ClampPositionIntoDocument(newPos);
	newPos = MovePositionOutsideChar(newPos, delta);
	if (!multipleSelection && sel.IsRectangular() && (selt == Selection::selStream)) {
		// Can't turn into multiple selection so clear additional selections
		InvalidateSelection(SelectionRange(newPos), true);
		SelectionRange rangeMain = sel.RangeMain();
		sel.SetSelection(rangeMain);
	}
	if (!sel.IsRectangular() && (selt == Selection::selRectangle)) {
		// Switching to rectangular
		InvalidateSelection(sel.RangeMain(), false);
		SelectionRange rangeMain = sel.RangeMain();
		sel.Clear();
		sel.Rectangular() = rangeMain;
	}
	if (selt != Selection::noSel) {
		sel.selType = selt;
	}
	if (selt != Selection::noSel || sel.MoveExtends()) {
		SetSelection(newPos);
	} else {
		SetEmptySelection(newPos);
	}
	ShowCaretAtCurrentPosition();

	int currentLine = pdoc->LineFromPosition(newPos.Position());
	if (ensureVisible) {
		// In case in need of wrapping to ensure DisplayFromDoc works.
		if (currentLine >= wrapPending.start)
			WrapLines(wsAll);
		XYScrollPosition newXY = XYScrollToMakeVisible(
			SelectionRange(posDrag.IsValid() ? posDrag : sel.RangeMain().caret), xysDefault);
		if (simpleCaret && (newXY.xOffset == xOffset)) {
			// simple vertical scroll then invalidate
			ScrollTo(newXY.topLine);
			InvalidateSelection(SelectionRange(spCaret), true);
		} else {
			SetXYScroll(newXY);
		}
	}

	if (marginView.highlightDelimiter.NeedsDrawing(currentLine)) {
		RedrawSelMargin();
	}
	return 0;
}

int Editor::MovePositionTo(int newPos, Selection::selTypes selt, bool ensureVisible) {
	return MovePositionTo(SelectionPosition(newPos), selt, ensureVisible);
}

SelectionPosition Editor::MovePositionSoVisible(SelectionPosition pos, int moveDir) {
	pos = ClampPositionIntoDocument(pos);
	pos = MovePositionOutsideChar(pos, moveDir);
	int lineDoc = pdoc->LineFromPosition(pos.Position());
	if (cs.GetVisible(lineDoc)) {
		return pos;
	} else {
		int lineDisplay = cs.DisplayFromDoc(lineDoc);
		if (moveDir > 0) {
			// lineDisplay is already line before fold as lines in fold use display line of line after fold
			lineDisplay = Platform::Clamp(lineDisplay, 0, cs.LinesDisplayed());
			return SelectionPosition(pdoc->LineStart(cs.DocFromDisplay(lineDisplay)));
		} else {
			lineDisplay = Platform::Clamp(lineDisplay - 1, 0, cs.LinesDisplayed());
			return SelectionPosition(pdoc->LineEnd(cs.DocFromDisplay(lineDisplay)));
		}
	}
}

SelectionPosition Editor::MovePositionSoVisible(int pos, int moveDir) {
	return MovePositionSoVisible(SelectionPosition(pos), moveDir);
}

Point Editor::PointMainCaret() {
	return LocationFromPosition(sel.Range(sel.Main()).caret);
}

/**
 * Choose the x position that the caret will try to stick to
 * as it moves up and down.
 */
void Editor::SetLastXChosen() {
	Point pt = PointMainCaret();
	lastXChosen = static_cast<int>(pt.x) + xOffset;
}

void Editor::ScrollTo(int line, bool moveThumb) {
	int topLineNew = Platform::Clamp(line, 0, MaxScrollPos());
	if (topLineNew != topLine) {
		// Try to optimise small scrolls
#ifndef UNDER_CE
		int linesToMove = topLine - topLineNew;
		bool performBlit = (abs(linesToMove) <= 10) && (paintState == notPainting);
		willRedrawAll = !performBlit;
#endif
		SetTopLine(topLineNew);
		// Optimize by styling the view as this will invalidate any needed area
		// which could abort the initial paint if discovered later.
		StyleToPositionInView(PositionAfterArea(GetClientRectangle()));
#ifndef UNDER_CE
		// Perform redraw rather than scroll if many lines would be redrawn anyway.
		if (performBlit) {
			ScrollText(linesToMove);
		} else {
			Redraw();
		}
		willRedrawAll = false;
#else
		Redraw();
#endif
		if (moveThumb) {
			SetVerticalScrollPos();
		}
	}
}

void Editor::ScrollText(int /* linesToMove */) {
	//Platform::DebugPrintf("Editor::ScrollText %d\n", linesToMove);
	Redraw();
}

void Editor::HorizontalScrollTo(int xPos) {
	//Platform::DebugPrintf("HorizontalScroll %d\n", xPos);
	if (xPos < 0)
		xPos = 0;
	if (!Wrapping() && (xOffset != xPos)) {
		xOffset = xPos;
		ContainerNeedsUpdate(SC_UPDATE_H_SCROLL);
		SetHorizontalScrollPos();
		RedrawRect(GetClientRectangle());
	}
}

void Editor::VerticalCentreCaret() {
	int lineDoc = pdoc->LineFromPosition(sel.IsRectangular() ? sel.Rectangular().caret.Position() : sel.MainCaret());
	int lineDisplay = cs.DisplayFromDoc(lineDoc);
	int newTop = lineDisplay - (LinesOnScreen() / 2);
	if (topLine != newTop) {
		SetTopLine(newTop > 0 ? newTop : 0);
		RedrawRect(GetClientRectangle());
	}
}

// Avoid 64 bit compiler warnings.
// Scintilla does not support text buffers larger than 2**31
static int istrlen(const char *s) {
	return static_cast<int>(s ? strlen(s) : 0);
}

void Editor::MoveSelectedLines(int lineDelta) {

	// if selection doesn't start at the beginning of the line, set the new start
	int selectionStart = SelectionStart().Position();
	int startLine = pdoc->LineFromPosition(selectionStart);
	int beginningOfStartLine = pdoc->LineStart(startLine);
	selectionStart = beginningOfStartLine;

	// if selection doesn't end at the beginning of a line greater than that of the start,
	// then set it at the beginning of the next one
	int selectionEnd = SelectionEnd().Position();
	int endLine = pdoc->LineFromPosition(selectionEnd);
	int beginningOfEndLine = pdoc->LineStart(endLine);
	bool appendEol = false;
	if (selectionEnd > beginningOfEndLine
		|| selectionStart == selectionEnd) {
		selectionEnd = pdoc->LineStart(endLine + 1);
		appendEol = (selectionEnd == pdoc->Length() && pdoc->LineFromPosition(selectionEnd) == endLine);
	}

	// if there's nowhere for the selection to move
	// (i.e. at the beginning going up or at the end going down),
	// stop it right there!
	if ((selectionStart == 0 && lineDelta < 0)
		|| (selectionEnd == pdoc->Length() && lineDelta > 0)
	        || selectionStart == selectionEnd) {
		return;
	}

	UndoGroup ug(pdoc);

	if (lineDelta > 0 && selectionEnd == pdoc->LineStart(pdoc->LinesTotal() - 1)) {
		SetSelection(pdoc->MovePositionOutsideChar(selectionEnd - 1, -1), selectionEnd);
		ClearSelection();
		selectionEnd = CurrentPosition();
	}
	SetSelection(selectionStart, selectionEnd);

	SelectionText selectedText;
	CopySelectionRange(&selectedText);

	int selectionLength = SelectionRange(selectionStart, selectionEnd).Length();
	Point currentLocation = LocationFromPosition(CurrentPosition());
	int currentLine = LineFromLocation(currentLocation);

	if (appendEol)
		SetSelection(pdoc->MovePositionOutsideChar(selectionStart - 1, -1), selectionEnd);
	ClearSelection();

	const char *eol = StringFromEOLMode(pdoc->eolMode);
	if (currentLine + lineDelta >= pdoc->LinesTotal())
		pdoc->InsertString(pdoc->Length(), eol, istrlen(eol));
	GoToLine(currentLine + lineDelta);

	selectionLength = pdoc->InsertString(CurrentPosition(), selectedText.Data(), selectionLength);
	if (appendEol) {
		const int lengthInserted = pdoc->InsertString(CurrentPosition() + selectionLength, eol, istrlen(eol));
		selectionLength += lengthInserted;
	}
	SetSelection(CurrentPosition(), CurrentPosition() + selectionLength);
}

void Editor::MoveSelectedLinesUp() {
	MoveSelectedLines(-1);
}

void Editor::MoveSelectedLinesDown() {
	MoveSelectedLines(1);
}

void Editor::MoveCaretInsideView(bool ensureVisible) {
	PRectangle rcClient = GetTextRectangle();
	Point pt = PointMainCaret();
	if (pt.y < rcClient.top) {
		MovePositionTo(SPositionFromLocation(
		            Point::FromInts(lastXChosen - xOffset, static_cast<int>(rcClient.top)),
					false, false, UserVirtualSpace()),
					Selection::noSel, ensureVisible);
	} else if ((pt.y + vs.lineHeight - 1) > rcClient.bottom) {
		int yOfLastLineFullyDisplayed = static_cast<int>(rcClient.top) + (LinesOnScreen() - 1) * vs.lineHeight;
		MovePositionTo(SPositionFromLocation(
		            Point::FromInts(lastXChosen - xOffset, static_cast<int>(rcClient.top) + yOfLastLineFullyDisplayed),
					false, false, UserVirtualSpace()),
		        Selection::noSel, ensureVisible);
	}
}

int Editor::DisplayFromPosition(int pos) {
	AutoSurface surface(this);
	return view.DisplayFromPosition(surface, *this, pos, vs);
}

/**
 * Ensure the caret is reasonably visible in context.
 *
Caret policy in SciTE

If slop is set, we can define a slop value.
This value defines an unwanted zone (UZ) where the caret is... unwanted.
This zone is defined as a number of pixels near the vertical margins,
and as a number of lines near the horizontal margins.
By keeping the caret away from the edges, it is seen within its context,
so it is likely that the identifier that the caret is on can be completely seen,
and that the current line is seen with some of the lines following it which are
often dependent on that line.

If strict is set, the policy is enforced... strictly.
The caret is centred on the display if slop is not set,
and cannot go in the UZ if slop is set.

If jumps is set, the display is moved more energetically
so the caret can move in the same direction longer before the policy is applied again.
'3UZ' notation is used to indicate three time the size of the UZ as a distance to the margin.

If even is not set, instead of having symmetrical UZs,
the left and bottom UZs are extended up to right and top UZs respectively.
This way, we favour the displaying of useful information: the beginning of lines,
where most code reside, and the lines after the caret, eg. the body of a function.

     |        |       |      |                                            |
slop | strict | jumps | even | Caret can go to the margin                 | When reaching limit (caret going out of
     |        |       |      |                                            | visibility or going into the UZ) display is...
-----+--------+-------+------+--------------------------------------------+--------------------------------------------------------------
  0  |   0    |   0   |   0  | Yes                                        | moved to put caret on top/on right
  0  |   0    |   0   |   1  | Yes                                        | moved by one position
  0  |   0    |   1   |   0  | Yes                                        | moved to put caret on top/on right
  0  |   0    |   1   |   1  | Yes                                        | centred on the caret
  0  |   1    |   -   |   0  | Caret is always on top/on right of display | -
  0  |   1    |   -   |   1  | No, caret is always centred                | -
  1  |   0    |   0   |   0  | Yes                                        | moved to put caret out of the asymmetrical UZ
  1  |   0    |   0   |   1  | Yes                                        | moved to put caret out of the UZ
  1  |   0    |   1   |   0  | Yes                                        | moved to put caret at 3UZ of the top or right margin
  1  |   0    |   1   |   1  | Yes                                        | moved to put caret at 3UZ of the margin
  1  |   1    |   -   |   0  | Caret is always at UZ of top/right margin  | -
  1  |   1    |   0   |   1  | No, kept out of UZ                         | moved by one position
  1  |   1    |   1   |   1  | No, kept out of UZ                         | moved to put caret at 3UZ of the margin
*/

Editor::XYScrollPosition Editor::XYScrollToMakeVisible(const SelectionRange &range, const XYScrollOptions options) {
	PRectangle rcClient = GetTextRectangle();
	Point pt = LocationFromPosition(range.caret);
	Point ptAnchor = LocationFromPosition(range.anchor);
	const Point ptOrigin = GetVisibleOriginInMain();
	pt.x += ptOrigin.x;
	pt.y += ptOrigin.y;
	ptAnchor.x += ptOrigin.x;
	ptAnchor.y += ptOrigin.y;
	const Point ptBottomCaret(pt.x, pt.y + vs.lineHeight - 1);

	XYScrollPosition newXY(xOffset, topLine);
	if (rcClient.Empty()) {
		return newXY;
	}

	// Vertical positioning
	if ((options & xysVertical) && (pt.y < rcClient.top || ptBottomCaret.y >= rcClient.bottom || (caretYPolicy & CARET_STRICT) != 0)) {
		const int lineCaret = DisplayFromPosition(range.caret.Position());
		const int linesOnScreen = LinesOnScreen();
		const int halfScreen = Platform::Maximum(linesOnScreen - 1, 2) / 2;
		const bool bSlop = (caretYPolicy & CARET_SLOP) != 0;
		const bool bStrict = (caretYPolicy & CARET_STRICT) != 0;
		const bool bJump = (caretYPolicy & CARET_JUMPS) != 0;
		const bool bEven = (caretYPolicy & CARET_EVEN) != 0;

		// It should be possible to scroll the window to show the caret,
		// but this fails to remove the caret on GTK+
		if (bSlop) {	// A margin is defined
			int yMoveT, yMoveB;
			if (bStrict) {
				int yMarginT, yMarginB;
				if (!(options & xysUseMargin)) {
					// In drag mode, avoid moves
					// otherwise, a double click will select several lines.
					yMarginT = yMarginB = 0;
				} else {
					// yMarginT must equal to caretYSlop, with a minimum of 1 and
					// a maximum of slightly less than half the heigth of the text area.
					yMarginT = Platform::Clamp(caretYSlop, 1, halfScreen);
					if (bEven) {
						yMarginB = yMarginT;
					} else {
						yMarginB = linesOnScreen - yMarginT - 1;
					}
				}
				yMoveT = yMarginT;
				if (bEven) {
					if (bJump) {
						yMoveT = Platform::Clamp(caretYSlop * 3, 1, halfScreen);
					}
					yMoveB = yMoveT;
				} else {
					yMoveB = linesOnScreen - yMoveT - 1;
				}
				if (lineCaret < topLine + yMarginT) {
					// Caret goes too high
					newXY.topLine = lineCaret - yMoveT;
				} else if (lineCaret > topLine + linesOnScreen - 1 - yMarginB) {
					// Caret goes too low
					newXY.topLine = lineCaret - linesOnScreen + 1 + yMoveB;
				}
			} else {	// Not strict
				yMoveT = bJump ? caretYSlop * 3 : caretYSlop;
				yMoveT = Platform::Clamp(yMoveT, 1, halfScreen);
				if (bEven) {
					yMoveB = yMoveT;
				} else {
					yMoveB = linesOnScreen - yMoveT - 1;
				}
				if (lineCaret < topLine) {
					// Caret goes too high
					newXY.topLine = lineCaret - yMoveT;
				} else if (lineCaret > topLine + linesOnScreen - 1) {
					// Caret goes too low
					newXY.topLine = lineCaret - linesOnScreen + 1 + yMoveB;
				}
			}
		} else {	// No slop
			if (!bStrict && !bJump) {
				// Minimal move
				if (lineCaret < topLine) {
					// Caret goes too high
					newXY.topLine = lineCaret;
				} else if (lineCaret > topLine + linesOnScreen - 1) {
					// Caret goes too low
					if (bEven) {
						newXY.topLine = lineCaret - linesOnScreen + 1;
					} else {
						newXY.topLine = lineCaret;
					}
				}
			} else {	// Strict or going out of display
				if (bEven) {
					// Always center caret
					newXY.topLine = lineCaret - halfScreen;
				} else {
					// Always put caret on top of display
					newXY.topLine = lineCaret;
				}
			}
		}
		if (!(range.caret == range.anchor)) {
			const int lineAnchor = DisplayFromPosition(range.anchor.Position());
			if (lineAnchor < lineCaret) {
				// Shift up to show anchor or as much of range as possible
				newXY.topLine = std::min(newXY.topLine, lineAnchor);
				newXY.topLine = std::max(newXY.topLine, lineCaret - LinesOnScreen());
			} else {
				// Shift down to show anchor or as much of range as possible
				newXY.topLine = std::max(newXY.topLine, lineAnchor - LinesOnScreen());
				newXY.topLine = std::min(newXY.topLine, lineCaret);
			}
		}
		newXY.topLine = Platform::Clamp(newXY.topLine, 0, MaxScrollPos());
	}

	// Horizontal positioning
	if ((options & xysHorizontal) && !Wrapping()) {
		const int halfScreen = Platform::Maximum(static_cast<int>(rcClient.Width()) - 4, 4) / 2;
		const bool bSlop = (caretXPolicy & CARET_SLOP) != 0;
		const bool bStrict = (caretXPolicy & CARET_STRICT) != 0;
		const bool bJump = (caretXPolicy & CARET_JUMPS) != 0;
		const bool bEven = (caretXPolicy & CARET_EVEN) != 0;

		if (bSlop) {	// A margin is defined
			int xMoveL, xMoveR;
			if (bStrict) {
				int xMarginL, xMarginR;
				if (!(options & xysUseMargin)) {
					// In drag mode, avoid moves unless very near of the margin
					// otherwise, a simple click will select text.
					xMarginL = xMarginR = 2;
				} else {
					// xMargin must equal to caretXSlop, with a minimum of 2 and
					// a maximum of slightly less than half the width of the text area.
					xMarginR = Platform::Clamp(caretXSlop, 2, halfScreen);
					if (bEven) {
						xMarginL = xMarginR;
					} else {
						xMarginL = static_cast<int>(rcClient.Width()) - xMarginR - 4;
					}
				}
				if (bJump && bEven) {
					// Jump is used only in even mode
					xMoveL = xMoveR = Platform::Clamp(caretXSlop * 3, 1, halfScreen);
				} else {
					xMoveL = xMoveR = 0;	// Not used, avoid a warning
				}
				if (pt.x < rcClient.left + xMarginL) {
					// Caret is on the left of the display
					if (bJump && bEven) {
						newXY.xOffset -= xMoveL;
					} else {
						// Move just enough to allow to display the caret
						newXY.xOffset -= static_cast<int>((rcClient.left + xMarginL) - pt.x);
					}
				} else if (pt.x >= rcClient.right - xMarginR) {
					// Caret is on the right of the display
					if (bJump && bEven) {
						newXY.xOffset += xMoveR;
					} else {
						// Move just enough to allow to display the caret
						newXY.xOffset += static_cast<int>(pt.x - (rcClient.right - xMarginR) + 1);
					}
				}
			} else {	// Not strict
				xMoveR = bJump ? caretXSlop * 3 : caretXSlop;
				xMoveR = Platform::Clamp(xMoveR, 1, halfScreen);
				if (bEven) {
					xMoveL = xMoveR;
				} else {
					xMoveL = static_cast<int>(rcClient.Width()) - xMoveR - 4;
				}
				if (pt.x < rcClient.left) {
					// Caret is on the left of the display
					newXY.xOffset -= xMoveL;
				} else if (pt.x >= rcClient.right) {
					// Caret is on the right of the display
					newXY.xOffset += xMoveR;
				}
			}
		} else {	// No slop
			if (bStrict ||
			        (bJump && (pt.x < rcClient.left || pt.x >= rcClient.right))) {
				// Strict or going out of display
				if (bEven) {
					// Center caret
					newXY.xOffset += static_cast<int>(pt.x - rcClient.left - halfScreen);
				} else {
					// Put caret on right
					newXY.xOffset += static_cast<int>(pt.x - rcClient.right + 1);
				}
			} else {
				// Move just enough to allow to display the caret
				if (pt.x < rcClient.left) {
					// Caret is on the left of the display
					if (bEven) {
						newXY.xOffset -= static_cast<int>(rcClient.left - pt.x);
					} else {
						newXY.xOffset += static_cast<int>(pt.x - rcClient.right) + 1;
					}
				} else if (pt.x >= rcClient.right) {
					// Caret is on the right of the display
					newXY.xOffset += static_cast<int>(pt.x - rcClient.right) + 1;
				}
			}
		}
		// In case of a jump (find result) largely out of display, adjust the offset to display the caret
		if (pt.x + xOffset < rcClient.left + newXY.xOffset) {
			newXY.xOffset = static_cast<int>(pt.x + xOffset - rcClient.left) - 2;
		} else if (pt.x + xOffset >= rcClient.right + newXY.xOffset) {
			newXY.xOffset = static_cast<int>(pt.x + xOffset - rcClient.right) + 2;
			if ((vs.caretStyle == CARETSTYLE_BLOCK) || view.imeCaretBlockOverride) {
				// Ensure we can see a good portion of the block caret
				newXY.xOffset += static_cast<int>(vs.aveCharWidth);
			}
		}
		if (!(range.caret == range.anchor)) {
			if (ptAnchor.x < pt.x) {
				// Shift to left to show anchor or as much of range as possible
				int maxOffset = static_cast<int>(ptAnchor.x + xOffset - rcClient.left) - 1;
				int minOffset = static_cast<int>(pt.x + xOffset - rcClient.right) + 1;
				newXY.xOffset = std::min(newXY.xOffset, maxOffset);
				newXY.xOffset = std::max(newXY.xOffset, minOffset);
			} else {
				// Shift to right to show anchor or as much of range as possible
				int minOffset = static_cast<int>(ptAnchor.x + xOffset - rcClient.right) + 1;
				int maxOffset = static_cast<int>(pt.x + xOffset - rcClient.left) - 1;
				newXY.xOffset = std::max(newXY.xOffset, minOffset);
				newXY.xOffset = std::min(newXY.xOffset, maxOffset);
			}
		}
		if (newXY.xOffset < 0) {
			newXY.xOffset = 0;
		}
	}

	return newXY;
}

void Editor::SetXYScroll(XYScrollPosition newXY) {
	if ((newXY.topLine != topLine) || (newXY.xOffset != xOffset)) {
		if (newXY.topLine != topLine) {
			SetTopLine(newXY.topLine);
			SetVerticalScrollPos();
		}
		if (newXY.xOffset != xOffset) {
			xOffset = newXY.xOffset;
			ContainerNeedsUpdate(SC_UPDATE_H_SCROLL);
			if (newXY.xOffset > 0) {
				PRectangle rcText = GetTextRectangle();
				if (horizontalScrollBarVisible &&
					rcText.Width() + xOffset > scrollWidth) {
					scrollWidth = xOffset + static_cast<int>(rcText.Width());
					SetScrollBars();
				}
			}
			SetHorizontalScrollPos();
		}
		Redraw();
		UpdateSystemCaret();
	}
}

void Editor::ScrollRange(SelectionRange range) {
	SetXYScroll(XYScrollToMakeVisible(range, xysDefault));
}

void Editor::EnsureCaretVisible(bool useMargin, bool vert, bool horiz) {
	SetXYScroll(XYScrollToMakeVisible(SelectionRange(posDrag.IsValid() ? posDrag : sel.RangeMain().caret),
		static_cast<XYScrollOptions>((useMargin?xysUseMargin:0)|(vert?xysVertical:0)|(horiz?xysHorizontal:0))));
}

void Editor::ShowCaretAtCurrentPosition() {
	if (hasFocus) {
		caret.active = true;
		caret.on = true;
		if (FineTickerAvailable()) {
			FineTickerCancel(tickCaret);
			if (caret.period > 0)
				FineTickerStart(tickCaret, caret.period, caret.period/10);
		} else {
			SetTicking(true);
		}
	} else {
		caret.active = false;
		caret.on = false;
		if (FineTickerAvailable()) {
			FineTickerCancel(tickCaret);
		}
	}
	InvalidateCaret();
}

void Editor::DropCaret() {
	caret.active = false;
	if (FineTickerAvailable()) {
		FineTickerCancel(tickCaret);
	}
	InvalidateCaret();
}

void Editor::CaretSetPeriod(int period) {
	if (caret.period != period) {
		caret.period = period;
		caret.on = true;
		if (FineTickerAvailable()) {
			FineTickerCancel(tickCaret);
			if ((caret.active) && (caret.period > 0))
				FineTickerStart(tickCaret, caret.period, caret.period/10);
		}
		InvalidateCaret();
	}
}

void Editor::InvalidateCaret() {
	if (posDrag.IsValid()) {
		InvalidateRange(posDrag.Position(), posDrag.Position() + 1);
	} else {
		for (size_t r=0; r<sel.Count(); r++) {
			InvalidateRange(sel.Range(r).caret.Position(), sel.Range(r).caret.Position() + 1);
		}
	}
	UpdateSystemCaret();
}

void Editor::UpdateSystemCaret() {
}

bool Editor::Wrapping() const {
	return vs.wrapState != eWrapNone;
}

void Editor::NeedWrapping(int docLineStart, int docLineEnd) {
//Platform::DebugPrintf("\nNeedWrapping: %0d..%0d\n", docLineStart, docLineEnd);
	if (wrapPending.AddRange(docLineStart, docLineEnd)) {
		view.llc.Invalidate(LineLayout::llPositions);
	}
	// Wrap lines during idle.
	if (Wrapping() && wrapPending.NeedsWrap()) {
		SetIdle(true);
	}
}

bool Editor::WrapOneLine(Surface *surface, int lineToWrap) {
	AutoLineLayout ll(view.llc, view.RetrieveLineLayout(lineToWrap, *this));
	int linesWrapped = 1;
	if (ll) {
		view.LayoutLine(*this, lineToWrap, surface, vs, ll, wrapWidth);
		linesWrapped = ll->lines;
	}
	return cs.SetHeight(lineToWrap, linesWrapped +
		(vs.annotationVisible ? pdoc->AnnotationLines(lineToWrap) : 0));
}

// Perform  wrapping for a subset of the lines needing wrapping.
// wsAll: wrap all lines which need wrapping in this single call
// wsVisible: wrap currently visible lines
// wsIdle: wrap one page + 100 lines
// Return true if wrapping occurred.
bool Editor::WrapLines(enum wrapScope ws) {
	int goodTopLine = topLine;
	bool wrapOccurred = false;
	if (!Wrapping()) {
		if (wrapWidth != LineLayout::wrapWidthInfinite) {
			wrapWidth = LineLayout::wrapWidthInfinite;
			for (int lineDoc = 0; lineDoc < pdoc->LinesTotal(); lineDoc++) {
				cs.SetHeight(lineDoc, 1 +
					(vs.annotationVisible ? pdoc->AnnotationLines(lineDoc) : 0));
			}
			wrapOccurred = true;
		}
		wrapPending.Reset();

	} else if (wrapPending.NeedsWrap()) {
		wrapPending.start = std::min(wrapPending.start, pdoc->LinesTotal());
		if (!SetIdle(true)) {
			// Idle processing not supported so full wrap required.
			ws = wsAll;
		}
		// Decide where to start wrapping
		int lineToWrap = wrapPending.start;
		int lineToWrapEnd = std::min(wrapPending.end, pdoc->LinesTotal());
		const int lineDocTop = cs.DocFromDisplay(topLine);
		const int subLineTop = topLine - cs.DisplayFromDoc(lineDocTop);
		if (ws == wsVisible) {
			lineToWrap = Platform::Clamp(lineDocTop-5, wrapPending.start, pdoc->LinesTotal());
			// Priority wrap to just after visible area.
			// Since wrapping could reduce display lines, treat each
			// as taking only one display line.
			lineToWrapEnd = lineDocTop;
			int lines = LinesOnScreen() + 1;
			while ((lineToWrapEnd < cs.LinesInDoc()) && (lines>0)) {
				if (cs.GetVisible(lineToWrapEnd))
					lines--;
				lineToWrapEnd++;
			}
			// .. and if the paint window is outside pending wraps
			if ((lineToWrap > wrapPending.end) || (lineToWrapEnd < wrapPending.start)) {
				// Currently visible text does not need wrapping
				return false;
			}
		} else if (ws == wsIdle) {
			lineToWrapEnd = lineToWrap + LinesOnScreen() + 100;
		}
		const int lineEndNeedWrap = std::min(wrapPending.end, pdoc->LinesTotal());
		lineToWrapEnd = std::min(lineToWrapEnd, lineEndNeedWrap);

		// Ensure all lines being wrapped are styled.
		pdoc->EnsureStyledTo(pdoc->LineStart(lineToWrapEnd));

		if (lineToWrap < lineToWrapEnd) {

			PRectangle rcTextArea = GetClientRectangle();
			rcTextArea.left = static_cast<XYPOSITION>(vs.textStart);
			rcTextArea.right -= vs.rightMarginWidth;
			wrapWidth = static_cast<int>(rcTextArea.Width());
			RefreshStyleData();
			AutoSurface surface(this);
			if (surface) {
//Platform::DebugPrintf("Wraplines: scope=%0d need=%0d..%0d perform=%0d..%0d\n", ws, wrapPending.start, wrapPending.end, lineToWrap, lineToWrapEnd);

				while (lineToWrap < lineToWrapEnd) {
					if (WrapOneLine(surface, lineToWrap)) {
						wrapOccurred = true;
					}
					wrapPending.Wrapped(lineToWrap);
					lineToWrap++;
				}

				goodTopLine = cs.DisplayFromDoc(lineDocTop) + std::min(subLineTop, cs.GetHeight(lineDocTop)-1);
			}
		}

		// If wrapping is done, bring it to resting position
		if (wrapPending.start >= lineEndNeedWrap) {
			wrapPending.Reset();
		}
	}

	if (wrapOccurred) {
		SetScrollBars();
		SetTopLine(Platform::Clamp(goodTopLine, 0, MaxScrollPos()));
		SetVerticalScrollPos();
	}

	return wrapOccurred;
}

void Editor::LinesJoin() {
	if (!RangeContainsProtected(targetStart, targetEnd)) {
		UndoGroup ug(pdoc);
		bool prevNonWS = true;
		for (int pos = targetStart; pos < targetEnd; pos++) {
			if (pdoc->IsPositionInLineEnd(pos)) {
				targetEnd -= pdoc->LenChar(pos);
				pdoc->DelChar(pos);
				if (prevNonWS) {
					// Ensure at least one space separating previous lines
					const int lengthInserted = pdoc->InsertString(pos, " ", 1);
					targetEnd += lengthInserted;
				}
			} else {
				prevNonWS = pdoc->CharAt(pos) != ' ';
			}
		}
	}
}

const char *Editor::StringFromEOLMode(int eolMode) {
	if (eolMode == SC_EOL_CRLF) {
		return "\r\n";
	} else if (eolMode == SC_EOL_CR) {
		return "\r";
	} else {
		return "\n";
	}
}

void Editor::LinesSplit(int pixelWidth) {
	if (!RangeContainsProtected(targetStart, targetEnd)) {
		if (pixelWidth == 0) {
			PRectangle rcText = GetTextRectangle();
			pixelWidth = static_cast<int>(rcText.Width());
		}
		int lineStart = pdoc->LineFromPosition(targetStart);
		int lineEnd = pdoc->LineFromPosition(targetEnd);
		const char *eol = StringFromEOLMode(pdoc->eolMode);
		UndoGroup ug(pdoc);
		for (int line = lineStart; line <= lineEnd; line++) {
			AutoSurface surface(this);
			AutoLineLayout ll(view.llc, view.RetrieveLineLayout(line, *this));
			if (surface && ll) {
				unsigned int posLineStart = pdoc->LineStart(line);
				view.LayoutLine(*this, line, surface, vs, ll, pixelWidth);
				int lengthInsertedTotal = 0;
				for (int subLine = 1; subLine < ll->lines; subLine++) {
					const int lengthInserted = pdoc->InsertString(
						static_cast<int>(posLineStart + lengthInsertedTotal +
							ll->LineStart(subLine)),
							eol, istrlen(eol));
					targetEnd += lengthInserted;
					lengthInsertedTotal += lengthInserted;
				}
			}
			lineEnd = pdoc->LineFromPosition(targetEnd);
		}
	}
}

void Editor::PaintSelMargin(Surface *surfWindow, PRectangle &rc) {
	if (vs.fixedColumnWidth == 0)
		return;

	AllocateGraphics();
	RefreshStyleData();
	RefreshPixMaps(surfWindow);

	// On GTK+ with Ubuntu overlay scroll bars, the surface may have been finished
	// at this point. The Initialised call checks for this case and sets the status
	// to be bad which avoids crashes in following calls.
	if (!surfWindow->Initialised()) {
		return;
	}

	PRectangle rcMargin = GetClientRectangle();
	Point ptOrigin = GetVisibleOriginInMain();
	rcMargin.Move(0, -ptOrigin.y);
	rcMargin.left = 0;
	rcMargin.right = static_cast<XYPOSITION>(vs.fixedColumnWidth);

	if (!rc.Intersects(rcMargin))
		return;

	Surface *surface;
	if (view.bufferedDraw) {
		surface = marginView.pixmapSelMargin;
	} else {
		surface = surfWindow;
	}

	// Clip vertically to paint area to avoid drawing line numbers
	if (rcMargin.bottom > rc.bottom)
		rcMargin.bottom = rc.bottom;
	if (rcMargin.top < rc.top)
		rcMargin.top = rc.top;

	marginView.PaintMargin(surface, topLine, rc, rcMargin, *this, vs);

	if (view.bufferedDraw) {
		surfWindow->Copy(rcMargin, Point(rcMargin.left, rcMargin.top), *marginView.pixmapSelMargin);
	}
}

void Editor::RefreshPixMaps(Surface *surfaceWindow) {
	view.RefreshPixMaps(surfaceWindow, wMain.GetID(), vs);
	marginView.RefreshPixMaps(surfaceWindow, wMain.GetID(), vs);
	if (view.bufferedDraw) {
		PRectangle rcClient = GetClientRectangle();
		if (!view.pixmapLine->Initialised()) {

			view.pixmapLine->InitPixMap(static_cast<int>(rcClient.Width()), vs.lineHeight,
			        surfaceWindow, wMain.GetID());
		}
		if (!marginView.pixmapSelMargin->Initialised()) {
			marginView.pixmapSelMargin->InitPixMap(vs.fixedColumnWidth,
				static_cast<int>(rcClient.Height()), surfaceWindow, wMain.GetID());
		}
	}
}

void Editor::Paint(Surface *surfaceWindow, PRectangle rcArea) {
	//Platform::DebugPrintf("Paint:%1d (%3d,%3d) ... (%3d,%3d)\n",
	//	paintingAllText, rcArea.left, rcArea.top, rcArea.right, rcArea.bottom);
	AllocateGraphics();

	RefreshStyleData();
	if (paintState == paintAbandoned)
		return;	// Scroll bars may have changed so need redraw
	RefreshPixMaps(surfaceWindow);

	paintAbandonedByStyling = false;

	StyleToPositionInView(PositionAfterArea(rcArea));

	PRectangle rcClient = GetClientRectangle();
	//Platform::DebugPrintf("Client: (%3d,%3d) ... (%3d,%3d)   %d\n",
	//	rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

	if (NotifyUpdateUI()) {
		RefreshStyleData();
		RefreshPixMaps(surfaceWindow);
	}

	// Wrap the visible lines if needed.
	if (WrapLines(wsVisible)) {
		// The wrapping process has changed the height of some lines so
		// abandon this paint for a complete repaint.
		if (AbandonPaint()) {
			return;
		}
		RefreshPixMaps(surfaceWindow);	// In case pixmaps invalidated by scrollbar change
	}
	PLATFORM_ASSERT(marginView.pixmapSelPattern->Initialised());

	if (!view.bufferedDraw)
		surfaceWindow->SetClip(rcArea);

	if (paintState != paintAbandoned) {
		if (vs.marginInside) {
			PaintSelMargin(surfaceWindow, rcArea);
			PRectangle rcRightMargin = rcClient;
			rcRightMargin.left = rcRightMargin.right - vs.rightMarginWidth;
			if (rcArea.Intersects(rcRightMargin)) {
				surfaceWindow->FillRectangle(rcRightMargin, vs.styles[STYLE_DEFAULT].back);
			}
		} else { // Else separate view so separate paint event but leftMargin included to allow overlap
			PRectangle rcLeftMargin = rcArea;
			rcLeftMargin.left = 0;
			rcLeftMargin.right = rcLeftMargin.left + vs.leftMarginWidth;
			if (rcArea.Intersects(rcLeftMargin)) {
				surfaceWindow->FillRectangle(rcLeftMargin, vs.styles[STYLE_DEFAULT].back);
			}
		}
	}

	if (paintState == paintAbandoned) {
		// Either styling or NotifyUpdateUI noticed that painting is needed
		// outside the current painting rectangle
		//Platform::DebugPrintf("Abandoning paint\n");
		if (Wrapping()) {
			if (paintAbandonedByStyling) {
				// Styling has spilled over a line end, such as occurs by starting a multiline
				// comment. The width of subsequent text may have changed, so rewrap.
				NeedWrapping(cs.DocFromDisplay(topLine));
			}
		}
		return;
	}

	view.PaintText(surfaceWindow, *this, rcArea, rcClient, vs);

	if (horizontalScrollBarVisible && trackLineWidth && (view.lineWidthMaxSeen > scrollWidth)) {
		if (FineTickerAvailable()) {
			scrollWidth = view.lineWidthMaxSeen;
			if (!FineTickerRunning(tickWiden)) {
				FineTickerStart(tickWiden, 50, 5);
			}
		}
	}

	NotifyPainted();
}

// This is mostly copied from the Paint method but with some things omitted
// such as the margin markers, line numbers, selection and caret
// Should be merged back into a combined Draw method.
long Editor::FormatRange(bool draw, Sci_RangeToFormat *pfr) {
	if (!pfr)
		return 0;

	AutoSurface surface(pfr->hdc, this, SC_TECHNOLOGY_DEFAULT);
	if (!surface)
		return 0;
	AutoSurface surfaceMeasure(pfr->hdcTarget, this, SC_TECHNOLOGY_DEFAULT);
	if (!surfaceMeasure) {
		return 0;
	}
	return view.FormatRange(draw, pfr, surface, surfaceMeasure, *this, vs);
}

int Editor::TextWidth(int style, const char *text) {
	RefreshStyleData();
	AutoSurface surface(this);
	if (surface) {
		return static_cast<int>(surface->WidthText(vs.styles[style].font, text, istrlen(text)));
	} else {
		return 1;
	}
}

// Empty method is overridden on GTK+ to show / hide scrollbars
void Editor::ReconfigureScrollBars() {}

void Editor::SetScrollBars() {
	RefreshStyleData();

	int nMax = MaxScrollPos();
	int nPage = LinesOnScreen();
	bool modified = ModifyScrollBars(nMax + nPage - 1, nPage);
	if (modified) {
		DwellEnd(true);
	}

	// TODO: ensure always showing as many lines as possible
	// May not be, if, for example, window made larger
	if (topLine > MaxScrollPos()) {
		SetTopLine(Platform::Clamp(topLine, 0, MaxScrollPos()));
		SetVerticalScrollPos();
		Redraw();
	}
	if (modified) {
		if (!AbandonPaint())
			Redraw();
	}
	//Platform::DebugPrintf("end max = %d page = %d\n", nMax, nPage);
}

void Editor::ChangeSize() {
	DropGraphics(false);
	SetScrollBars();
	if (Wrapping()) {
		PRectangle rcTextArea = GetClientRectangle();
		rcTextArea.left = static_cast<XYPOSITION>(vs.textStart);
		rcTextArea.right -= vs.rightMarginWidth;
		if (wrapWidth != rcTextArea.Width()) {
			NeedWrapping();
			Redraw();
		}
	}
}

int Editor::InsertSpace(int position, unsigned int spaces) {
	if (spaces > 0) {
		std::string spaceText(spaces, ' ');
		const int lengthInserted = pdoc->InsertString(position, spaceText.c_str(), spaces);
		position += lengthInserted;
	}
	return position;
}

void Editor::AddChar(char ch) {
	char s[2];
	s[0] = ch;
	s[1] = '\0';
	AddCharUTF(s, 1);
}

void Editor::FilterSelections() {
	if (!additionalSelectionTyping && (sel.Count() > 1)) {
		SelectionRange rangeOnly = sel.RangeMain();
		InvalidateSelection(rangeOnly, true);
		sel.SetSelection(rangeOnly);
	}
}

static bool cmpSelPtrs(const SelectionRange *a, const SelectionRange *b) {
	return *a < *b;
}

// AddCharUTF inserts an array of bytes which may or may not be in UTF-8.
void Editor::AddCharUTF(const char *s, unsigned int len, bool treatAsDBCS) {
	FilterSelections();
	{
		UndoGroup ug(pdoc, (sel.Count() > 1) || !sel.Empty() || inOverstrike);

		// Vector elements point into selection in order to change selection.
		std::vector<SelectionRange *> selPtrs;
		for (size_t r = 0; r < sel.Count(); r++) {
			selPtrs.push_back(&sel.Range(r));
		}
		// Order selections by position in document.
		std::sort(selPtrs.begin(), selPtrs.end(), cmpSelPtrs);

		// Loop in reverse to avoid disturbing positions of selections yet to be processed.
		for (std::vector<SelectionRange *>::reverse_iterator rit = selPtrs.rbegin();
			rit != selPtrs.rend(); ++rit) {
			SelectionRange *currentSel = *rit;
			if (!RangeContainsProtected(currentSel->Start().Position(),
				currentSel->End().Position())) {
				int positionInsert = currentSel->Start().Position();
				if (!currentSel->Empty()) {
					if (currentSel->Length()) {
						pdoc->DeleteChars(positionInsert, currentSel->Length());
						currentSel->ClearVirtualSpace();
					} else {
						// Range is all virtual so collapse to start of virtual space
						currentSel->MinimizeVirtualSpace();
					}
				} else if (inOverstrike) {
					if (positionInsert < pdoc->Length()) {
						if (!pdoc->IsPositionInLineEnd(positionInsert)) {
							pdoc->DelChar(positionInsert);
							currentSel->ClearVirtualSpace();
						}
					}
				}
				positionInsert = InsertSpace(positionInsert, currentSel->caret.VirtualSpace());
				const int lengthInserted = pdoc->InsertString(positionInsert, s, len);
				if (lengthInserted > 0) {
					currentSel->caret.SetPosition(positionInsert + lengthInserted);
					currentSel->anchor.SetPosition(positionInsert + lengthInserted);
				}
				currentSel->ClearVirtualSpace();
				// If in wrap mode rewrap current line so EnsureCaretVisible has accurate information
				if (Wrapping()) {
					AutoSurface surface(this);
					if (surface) {
						if (WrapOneLine(surface, pdoc->LineFromPosition(positionInsert))) {
							SetScrollBars();
							SetVerticalScrollPos();
							Redraw();
						}
					}
				}
			}
		}
	}
	if (Wrapping()) {
		SetScrollBars();
	}
	ThinRectangularRange();
	// If in wrap mode rewrap current line so EnsureCaretVisible has accurate information
	EnsureCaretVisible();
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
	if ((caretSticky == SC_CARETSTICKY_OFF) ||
		((caretSticky == SC_CARETSTICKY_WHITESPACE) && !IsAllSpacesOrTabs(s, len))) {
		SetLastXChosen();
	}

	if (treatAsDBCS) {
		NotifyChar((static_cast<unsigned char>(s[0]) << 8) |
		        static_cast<unsigned char>(s[1]));
	} else if (len > 0) {
		int byte = static_cast<unsigned char>(s[0]);
		if ((byte < 0xC0) || (1 == len)) {
			// Handles UTF-8 characters between 0x01 and 0x7F and single byte
			// characters when not in UTF-8 mode.
			// Also treats \0 and naked trail bytes 0x80 to 0xBF as valid
			// characters representing themselves.
		} else {
			unsigned int utf32[1] = { 0 };
			UTF32FromUTF8(s, len, utf32, ELEMENTS(utf32));
			byte = utf32[0];
		}
		NotifyChar(byte);
	}

	if (recordingMacro) {
		NotifyMacroRecord(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(s));
	}
}

void Editor::FillVirtualSpace() {
	const bool tmpOverstrike = inOverstrike;
	inOverstrike = false;         // not allow to be deleted twice.
	AddCharUTF("", 0);
	inOverstrike = tmpOverstrike;
}

void Editor::InsertPaste(const char *text, int len) {
	if (multiPasteMode == SC_MULTIPASTE_ONCE) {
		SelectionPosition selStart = sel.Start();
		selStart = SelectionPosition(InsertSpace(selStart.Position(), selStart.VirtualSpace()));
		const int lengthInserted = pdoc->InsertString(selStart.Position(), text, len);
		if (lengthInserted > 0) {
			SetEmptySelection(selStart.Position() + lengthInserted);
		}
	} else {
		// SC_MULTIPASTE_EACH
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r).Start().Position(),
				sel.Range(r).End().Position())) {
				int positionInsert = sel.Range(r).Start().Position();
				if (!sel.Range(r).Empty()) {
					if (sel.Range(r).Length()) {
						pdoc->DeleteChars(positionInsert, sel.Range(r).Length());
						sel.Range(r).ClearVirtualSpace();
					} else {
						// Range is all virtual so collapse to start of virtual space
						sel.Range(r).MinimizeVirtualSpace();
					}
				}
				positionInsert = InsertSpace(positionInsert, sel.Range(r).caret.VirtualSpace());
				const int lengthInserted = pdoc->InsertString(positionInsert, text, len);
				if (lengthInserted > 0) {
					sel.Range(r).caret.SetPosition(positionInsert + lengthInserted);
					sel.Range(r).anchor.SetPosition(positionInsert + lengthInserted);
				}
				sel.Range(r).ClearVirtualSpace();
			}
		}
	}
}

void Editor::InsertPasteShape(const char *text, int len, PasteShape shape) {
	std::string convertedText;
	if (convertPastes) {
		// Convert line endings of the paste into our local line-endings mode
		convertedText = Document::TransformLineEnds(text, len, pdoc->eolMode);
		len = static_cast<int>(convertedText.length());
		text = convertedText.c_str();
	}
	if (shape == pasteRectangular) {
		PasteRectangular(sel.Start(), text, len);
	} else {
		if (shape == pasteLine) {
			int insertPos = pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret()));
			int lengthInserted = pdoc->InsertString(insertPos, text, len);
			// add the newline if necessary
			if ((len > 0) && (text[len - 1] != '\n' && text[len - 1] != '\r')) {
				const char *endline = StringFromEOLMode(pdoc->eolMode);
				int length = static_cast<int>(strlen(endline));
				lengthInserted += pdoc->InsertString(insertPos + lengthInserted, endline, length);
			}
			if (sel.MainCaret() == insertPos) {
				SetEmptySelection(sel.MainCaret() + lengthInserted);
			}
		} else {
			InsertPaste(text, len);
		}
	}
}

void Editor::ClearSelection(bool retainMultipleSelections) {
	if (!sel.IsRectangular() && !retainMultipleSelections)
		FilterSelections();
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		if (!sel.Range(r).Empty()) {
			if (!RangeContainsProtected(sel.Range(r).Start().Position(),
				sel.Range(r).End().Position())) {
				pdoc->DeleteChars(sel.Range(r).Start().Position(),
					sel.Range(r).Length());
				sel.Range(r) = SelectionRange(sel.Range(r).Start());
			}
		}
	}
	ThinRectangularRange();
	sel.RemoveDuplicates();
	ClaimSelection();
	SetHoverIndicatorPosition(sel.MainCaret());
}

void Editor::ClearAll() {
	{
		UndoGroup ug(pdoc);
		if (0 != pdoc->Length()) {
			pdoc->DeleteChars(0, pdoc->Length());
		}
		if (!pdoc->IsReadOnly()) {
			cs.Clear();
			pdoc->AnnotationClearAll();
			pdoc->MarginClearAll();
		}
	}

	view.ClearAllTabstops();

	sel.Clear();
	SetTopLine(0);
	SetVerticalScrollPos();
	InvalidateStyleRedraw();
}

void Editor::ClearDocumentStyle() {
	Decoration *deco = pdoc->decorations.root;
	while (deco) {
		// Save next in case deco deleted
		Decoration *decoNext = deco->next;
		if (deco->indicator < INDIC_CONTAINER) {
			pdoc->decorations.SetCurrentIndicator(deco->indicator);
			pdoc->DecorationFillRange(0, 0, pdoc->Length());
		}
		deco = decoNext;
	}
	pdoc->StartStyling(0, '\377');
	pdoc->SetStyleFor(pdoc->Length(), 0);
	cs.ShowAll();
	SetAnnotationHeights(0, pdoc->LinesTotal());
	pdoc->ClearLevels();
}

void Editor::CopyAllowLine() {
	SelectionText selectedText;
	CopySelectionRange(&selectedText, true);
	CopyToClipboard(selectedText);
}

void Editor::Cut() {
	pdoc->CheckReadOnly();
	if (!pdoc->IsReadOnly() && !SelectionContainsProtected()) {
		Copy();
		ClearSelection();
	}
}

void Editor::PasteRectangular(SelectionPosition pos, const char *ptr, int len) {
	if (pdoc->IsReadOnly() || SelectionContainsProtected()) {
		return;
	}
	sel.Clear();
	sel.RangeMain() = SelectionRange(pos);
	int line = pdoc->LineFromPosition(sel.MainCaret());
	UndoGroup ug(pdoc);
	sel.RangeMain().caret = SelectionPosition(
		InsertSpace(sel.RangeMain().caret.Position(), sel.RangeMain().caret.VirtualSpace()));
	int xInsert = XFromPosition(sel.RangeMain().caret);
	bool prevCr = false;
	while ((len > 0) && IsEOLChar(ptr[len-1]))
		len--;
	for (int i = 0; i < len; i++) {
		if (IsEOLChar(ptr[i])) {
			if ((ptr[i] == '\r') || (!prevCr))
				line++;
			if (line >= pdoc->LinesTotal()) {
				if (pdoc->eolMode != SC_EOL_LF)
					pdoc->InsertString(pdoc->Length(), "\r", 1);
				if (pdoc->eolMode != SC_EOL_CR)
					pdoc->InsertString(pdoc->Length(), "\n", 1);
			}
			// Pad the end of lines with spaces if required
			sel.RangeMain().caret.SetPosition(PositionFromLineX(line, xInsert));
			if ((XFromPosition(sel.MainCaret()) < xInsert) && (i + 1 < len)) {
				while (XFromPosition(sel.MainCaret()) < xInsert) {
					const int lengthInserted = pdoc->InsertString(sel.MainCaret(), " ", 1);
					sel.RangeMain().caret.Add(lengthInserted);
				}
			}
			prevCr = ptr[i] == '\r';
		} else {
			const int lengthInserted = pdoc->InsertString(sel.MainCaret(), ptr + i, 1);
			sel.RangeMain().caret.Add(lengthInserted);
			prevCr = false;
		}
	}
	SetEmptySelection(pos);
}

bool Editor::CanPaste() {
	return !pdoc->IsReadOnly() && !SelectionContainsProtected();
}

void Editor::Clear() {
	// If multiple selections, don't delete EOLS
	if (sel.Empty()) {
		bool singleVirtual = false;
		if ((sel.Count() == 1) &&
			!RangeContainsProtected(sel.MainCaret(), sel.MainCaret() + 1) &&
			sel.RangeMain().Start().VirtualSpace()) {
			singleVirtual = true;
		}
		UndoGroup ug(pdoc, (sel.Count() > 1) || singleVirtual);
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r).caret.Position(), sel.Range(r).caret.Position() + 1)) {
				if (sel.Range(r).Start().VirtualSpace()) {
					if (sel.Range(r).anchor < sel.Range(r).caret)
						sel.Range(r) = SelectionRange(InsertSpace(sel.Range(r).anchor.Position(), sel.Range(r).anchor.VirtualSpace()));
					else
						sel.Range(r) = SelectionRange(InsertSpace(sel.Range(r).caret.Position(), sel.Range(r).caret.VirtualSpace()));
				}
				if ((sel.Count() == 1) || !pdoc->IsPositionInLineEnd(sel.Range(r).caret.Position())) {
					pdoc->DelChar(sel.Range(r).caret.Position());
					sel.Range(r).ClearVirtualSpace();
				}  // else multiple selection so don't eat line ends
			} else {
				sel.Range(r).ClearVirtualSpace();
			}
		}
	} else {
		ClearSelection();
	}
	sel.RemoveDuplicates();
	ShowCaretAtCurrentPosition();		// Avoid blinking
}

void Editor::SelectAll() {
	sel.Clear();
	SetSelection(0, pdoc->Length());
	Redraw();
}

void Editor::Undo() {
	if (pdoc->CanUndo()) {
		InvalidateCaret();
		int newPos = pdoc->Undo();
		if (newPos >= 0)
			SetEmptySelection(newPos);
		EnsureCaretVisible();
	}
}

void Editor::Redo() {
	if (pdoc->CanRedo()) {
		int newPos = pdoc->Redo();
		if (newPos >= 0)
			SetEmptySelection(newPos);
		EnsureCaretVisible();
	}
}

void Editor::DelCharBack(bool allowLineStartDeletion) {
	RefreshStyleData();
	if (!sel.IsRectangular())
		FilterSelections();
	if (sel.IsRectangular())
		allowLineStartDeletion = false;
	UndoGroup ug(pdoc, (sel.Count() > 1) || !sel.Empty());
	if (sel.Empty()) {
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r).caret.Position() - 1, sel.Range(r).caret.Position())) {
				if (sel.Range(r).caret.VirtualSpace()) {
					sel.Range(r).caret.SetVirtualSpace(sel.Range(r).caret.VirtualSpace() - 1);
					sel.Range(r).anchor.SetVirtualSpace(sel.Range(r).caret.VirtualSpace());
				} else {
					int lineCurrentPos = pdoc->LineFromPosition(sel.Range(r).caret.Position());
					if (allowLineStartDeletion || (pdoc->LineStart(lineCurrentPos) != sel.Range(r).caret.Position())) {
						if (pdoc->GetColumn(sel.Range(r).caret.Position()) <= pdoc->GetLineIndentation(lineCurrentPos) &&
								pdoc->GetColumn(sel.Range(r).caret.Position()) > 0 && pdoc->backspaceUnindents) {
							UndoGroup ugInner(pdoc, !ug.Needed());
							int indentation = pdoc->GetLineIndentation(lineCurrentPos);
							int indentationStep = pdoc->IndentSize();
							int indentationChange = indentation % indentationStep;
							if (indentationChange == 0)
								indentationChange = indentationStep;
							const int posSelect = pdoc->SetLineIndentation(lineCurrentPos, indentation - indentationChange);
							// SetEmptySelection
							sel.Range(r) = SelectionRange(posSelect);
						} else {
							pdoc->DelCharBack(sel.Range(r).caret.Position());
						}
					}
				}
			} else {
				sel.Range(r).ClearVirtualSpace();
			}
		}
		ThinRectangularRange();
	} else {
		ClearSelection();
	}
	sel.RemoveDuplicates();
	ContainerNeedsUpdate(SC_UPDATE_SELECTION);
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

int Editor::ModifierFlags(bool shift, bool ctrl, bool alt, bool meta) {
	return
		(shift ? SCI_SHIFT : 0) |
		(ctrl ? SCI_CTRL : 0) |
		(alt ? SCI_ALT : 0) |
		(meta ? SCI_META : 0);
}

void Editor::NotifyFocus(bool focus) {
	SCNotification scn = {};
	scn.nmhdr.code = focus ? SCN_FOCUSIN : SCN_FOCUSOUT;
	NotifyParent(scn);
}

void Editor::SetCtrlID(int identifier) {
	ctrlID = identifier;
}

void Editor::NotifyStyleToNeeded(int endStyleNeeded) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_STYLENEEDED;
	scn.position = endStyleNeeded;
	NotifyParent(scn);
}

void Editor::NotifyStyleNeeded(Document *, void *, int endStyleNeeded) {
	NotifyStyleToNeeded(endStyleNeeded);
}

void Editor::NotifyLexerChanged(Document *, void *) {
}

void Editor::NotifyErrorOccurred(Document *, void *, int status) {
	errorStatus = status;
}

void Editor::NotifyChar(int ch) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_CHARADDED;
	scn.ch = ch;
	NotifyParent(scn);
}

void Editor::NotifySavePoint(bool isSavePoint) {
	SCNotification scn = {};
	if (isSavePoint) {
		scn.nmhdr.code = SCN_SAVEPOINTREACHED;
	} else {
		scn.nmhdr.code = SCN_SAVEPOINTLEFT;
	}
	NotifyParent(scn);
}

void Editor::NotifyModifyAttempt() {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_MODIFYATTEMPTRO;
	NotifyParent(scn);
}

void Editor::NotifyDoubleClick(Point pt, int modifiers) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_DOUBLECLICK;
	scn.line = LineFromLocation(pt);
	scn.position = PositionFromLocation(pt, true);
	scn.modifiers = modifiers;
	NotifyParent(scn);
}

void Editor::NotifyDoubleClick(Point pt, bool shift, bool ctrl, bool alt) {
	NotifyDoubleClick(pt, ModifierFlags(shift, ctrl, alt));
}

void Editor::NotifyHotSpotDoubleClicked(int position, int modifiers) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_HOTSPOTDOUBLECLICK;
	scn.position = position;
	scn.modifiers = modifiers;
	NotifyParent(scn);
}

void Editor::NotifyHotSpotDoubleClicked(int position, bool shift, bool ctrl, bool alt) {
	NotifyHotSpotDoubleClicked(position, ModifierFlags(shift, ctrl, alt));
}

void Editor::NotifyHotSpotClicked(int position, int modifiers) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_HOTSPOTCLICK;
	scn.position = position;
	scn.modifiers = modifiers;
	NotifyParent(scn);
}

void Editor::NotifyHotSpotClicked(int position, bool shift, bool ctrl, bool alt) {
	NotifyHotSpotClicked(position, ModifierFlags(shift, ctrl, alt));
}

void Editor::NotifyHotSpotReleaseClick(int position, int modifiers) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_HOTSPOTRELEASECLICK;
	scn.position = position;
	scn.modifiers = modifiers;
	NotifyParent(scn);
}

void Editor::NotifyHotSpotReleaseClick(int position, bool shift, bool ctrl, bool alt) {
	NotifyHotSpotReleaseClick(position, ModifierFlags(shift, ctrl, alt));
}

bool Editor::NotifyUpdateUI() {
	if (needUpdateUI) {
		SCNotification scn = {};
		scn.nmhdr.code = SCN_UPDATEUI;
		scn.updated = needUpdateUI;
		NotifyParent(scn);
		needUpdateUI = 0;
		return true;
	}
	return false;
}

void Editor::NotifyPainted() {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_PAINTED;
	NotifyParent(scn);
}

void Editor::NotifyIndicatorClick(bool click, int position, int modifiers) {
	int mask = pdoc->decorations.AllOnFor(position);
	if ((click && mask) || pdoc->decorations.clickNotified) {
		SCNotification scn = {};
		pdoc->decorations.clickNotified = click;
		scn.nmhdr.code = click ? SCN_INDICATORCLICK : SCN_INDICATORRELEASE;
		scn.modifiers = modifiers;
		scn.position = position;
		NotifyParent(scn);
	}
}

void Editor::NotifyIndicatorClick(bool click, int position, bool shift, bool ctrl, bool alt) {
	NotifyIndicatorClick(click, position, ModifierFlags(shift, ctrl, alt));
}

bool Editor::NotifyMarginClick(Point pt, int modifiers) {
	int marginClicked = -1;
	int x = vs.textStart - vs.fixedColumnWidth;
	for (int margin = 0; margin <= SC_MAX_MARGIN; margin++) {
		if ((pt.x >= x) && (pt.x < x + vs.ms[margin].width))
			marginClicked = margin;
		x += vs.ms[margin].width;
	}
	if ((marginClicked >= 0) && vs.ms[marginClicked].sensitive) {
		int position = pdoc->LineStart(LineFromLocation(pt));
		if ((vs.ms[marginClicked].mask & SC_MASK_FOLDERS) && (foldAutomatic & SC_AUTOMATICFOLD_CLICK)) {
			const bool ctrl = (modifiers & SCI_CTRL) != 0;
			const bool shift = (modifiers & SCI_SHIFT) != 0;
			int lineClick = pdoc->LineFromPosition(position);
			if (shift && ctrl) {
				FoldAll(SC_FOLDACTION_TOGGLE);
			} else {
				int levelClick = pdoc->GetLevel(lineClick);
				if (levelClick & SC_FOLDLEVELHEADERFLAG) {
					if (shift) {
						// Ensure all children visible
						FoldExpand(lineClick, SC_FOLDACTION_EXPAND, levelClick);
					} else if (ctrl) {
						FoldExpand(lineClick, SC_FOLDACTION_TOGGLE, levelClick);
					} else {
						// Toggle this line
						FoldLine(lineClick, SC_FOLDACTION_TOGGLE);
					}
				}
			}
			return true;
		}
		SCNotification scn = {};
		scn.nmhdr.code = SCN_MARGINCLICK;
		scn.modifiers = modifiers;
		scn.position = position;
		scn.margin = marginClicked;
		NotifyParent(scn);
		return true;
	} else {
		return false;
	}
}

bool Editor::NotifyMarginClick(Point pt, bool shift, bool ctrl, bool alt) {
	return NotifyMarginClick(pt, ModifierFlags(shift, ctrl, alt));
}

void Editor::NotifyNeedShown(int pos, int len) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_NEEDSHOWN;
	scn.position = pos;
	scn.length = len;
	NotifyParent(scn);
}

void Editor::NotifyDwelling(Point pt, bool state) {
	SCNotification scn = {};
	scn.nmhdr.code = state ? SCN_DWELLSTART : SCN_DWELLEND;
	scn.position = PositionFromLocation(pt, true);
	scn.x = static_cast<int>(pt.x + vs.ExternalMarginWidth());
	scn.y = static_cast<int>(pt.y);
	NotifyParent(scn);
}

void Editor::NotifyZoom() {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_ZOOM;
	NotifyParent(scn);
}

// Notifications from document
void Editor::NotifyModifyAttempt(Document *, void *) {
	//Platform::DebugPrintf("** Modify Attempt\n");
	NotifyModifyAttempt();
}

void Editor::NotifySavePoint(Document *, void *, bool atSavePoint) {
	//Platform::DebugPrintf("** Save Point %s\n", atSavePoint ? "On" : "Off");
	NotifySavePoint(atSavePoint);
}

void Editor::CheckModificationForWrap(DocModification mh) {
	if (mh.modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
		view.llc.Invalidate(LineLayout::llCheckTextAndStyle);
		int lineDoc = pdoc->LineFromPosition(mh.position);
		int lines = Platform::Maximum(0, mh.linesAdded);
		if (Wrapping()) {
			NeedWrapping(lineDoc, lineDoc + lines + 1);
		}
		RefreshStyleData();
		// Fix up annotation heights
		SetAnnotationHeights(lineDoc, lineDoc + lines + 2);
	}
}

// Move a position so it is still after the same character as before the insertion.
static inline int MovePositionForInsertion(int position, int startInsertion, int length) {
	if (position > startInsertion) {
		return position + length;
	}
	return position;
}

// Move a position so it is still after the same character as before the deletion if that
// character is still present else after the previous surviving character.
static inline int MovePositionForDeletion(int position, int startDeletion, int length) {
	if (position > startDeletion) {
		int endDeletion = startDeletion + length;
		if (position > endDeletion) {
			return position - length;
		} else {
			return startDeletion;
		}
	} else {
		return position;
	}
}

void Editor::NotifyModified(Document *, DocModification mh, void *) {
	ContainerNeedsUpdate(SC_UPDATE_CONTENT);
	if (paintState == painting) {
		CheckForChangeOutsidePaint(Range(mh.position, mh.position + mh.length));
	}
	if (mh.modificationType & SC_MOD_CHANGELINESTATE) {
		if (paintState == painting) {
			CheckForChangeOutsidePaint(
			    Range(pdoc->LineStart(mh.line), pdoc->LineStart(mh.line + 1)));
		} else {
			// Could check that change is before last visible line.
			Redraw();
		}
	}
	if (mh.modificationType & SC_MOD_CHANGETABSTOPS) {
		Redraw();
	}
	if (mh.modificationType & SC_MOD_LEXERSTATE) {
		if (paintState == painting) {
			CheckForChangeOutsidePaint(
			    Range(mh.position, mh.position + mh.length));
		} else {
			Redraw();
		}
	}
	if (mh.modificationType & (SC_MOD_CHANGESTYLE | SC_MOD_CHANGEINDICATOR)) {
		if (mh.modificationType & SC_MOD_CHANGESTYLE) {
			pdoc->IncrementStyleClock();
		}
		if (paintState == notPainting) {
			if (mh.position < pdoc->LineStart(topLine)) {
				// Styling performed before this view
				Redraw();
			} else {
				InvalidateRange(mh.position, mh.position + mh.length);
			}
		}
		if (mh.modificationType & SC_MOD_CHANGESTYLE) {
			view.llc.Invalidate(LineLayout::llCheckTextAndStyle);
		}
	} else {
		// Move selection and brace highlights
		if (mh.modificationType & SC_MOD_INSERTTEXT) {
			sel.MovePositions(true, mh.position, mh.length);
			braces[0] = MovePositionForInsertion(braces[0], mh.position, mh.length);
			braces[1] = MovePositionForInsertion(braces[1], mh.position, mh.length);
		} else if (mh.modificationType & SC_MOD_DELETETEXT) {
			sel.MovePositions(false, mh.position, mh.length);
			braces[0] = MovePositionForDeletion(braces[0], mh.position, mh.length);
			braces[1] = MovePositionForDeletion(braces[1], mh.position, mh.length);
		}
		if ((mh.modificationType & (SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE)) && cs.HiddenLines()) {
			// Some lines are hidden so may need shown.
			// TODO: check if the modified area is hidden.
			if (mh.modificationType & SC_MOD_BEFOREINSERT) {
				int lineOfPos = pdoc->LineFromPosition(mh.position);
				bool insertingNewLine = false;
				for (int i=0; i < mh.length; i++) {
					if ((mh.text[i] == '\n') || (mh.text[i] == '\r'))
						insertingNewLine = true;
				}
				if (insertingNewLine && (mh.position != pdoc->LineStart(lineOfPos)))
					NeedShown(mh.position, pdoc->LineStart(lineOfPos+1) - mh.position);
				else
					NeedShown(mh.position, 0);
			} else if (mh.modificationType & SC_MOD_BEFOREDELETE) {
				NeedShown(mh.position, mh.length);
			}
		}
		if (mh.linesAdded != 0) {
			// Update contraction state for inserted and removed lines
			// lineOfPos should be calculated in context of state before modification, shouldn't it
			int lineOfPos = pdoc->LineFromPosition(mh.position);
			if (mh.position > pdoc->LineStart(lineOfPos))
				lineOfPos++;	// Affecting subsequent lines
			if (mh.linesAdded > 0) {
				cs.InsertLines(lineOfPos, mh.linesAdded);
			} else {
				cs.DeleteLines(lineOfPos, -mh.linesAdded);
			}
			view.LinesAddedOrRemoved(lineOfPos, mh.linesAdded);
		}
		if (mh.modificationType & SC_MOD_CHANGEANNOTATION) {
			int lineDoc = pdoc->LineFromPosition(mh.position);
			if (vs.annotationVisible) {
				cs.SetHeight(lineDoc, cs.GetHeight(lineDoc) + mh.annotationLinesAdded);
				Redraw();
			}
		}
		CheckModificationForWrap(mh);
		if (mh.linesAdded != 0) {
			// Avoid scrolling of display if change before current display
			if (mh.position < posTopLine && !CanDeferToLastStep(mh)) {
				int newTop = Platform::Clamp(topLine + mh.linesAdded, 0, MaxScrollPos());
				if (newTop != topLine) {
					SetTopLine(newTop);
					SetVerticalScrollPos();
				}
			}

			if (paintState == notPainting && !CanDeferToLastStep(mh)) {
				QueueIdleWork(WorkNeeded::workStyle, pdoc->Length());
				Redraw();
			}
		} else {
			if (paintState == notPainting && mh.length && !CanEliminate(mh)) {
				QueueIdleWork(WorkNeeded::workStyle, mh.position + mh.length);
				InvalidateRange(mh.position, mh.position + mh.length);
			}
		}
	}

	if (mh.linesAdded != 0 && !CanDeferToLastStep(mh)) {
		SetScrollBars();
	}

	if ((mh.modificationType & SC_MOD_CHANGEMARKER) || (mh.modificationType & SC_MOD_CHANGEMARGIN)) {
		if ((!willRedrawAll) && ((paintState == notPainting) || !PaintContainsMargin())) {
			if (mh.modificationType & SC_MOD_CHANGEFOLD) {
				// Fold changes can affect the drawing of following lines so redraw whole margin
				RedrawSelMargin(marginView.highlightDelimiter.isEnabled ? -1 : mh.line - 1, true);
			} else {
				RedrawSelMargin(mh.line);
			}
		}
	}
	if ((mh.modificationType & SC_MOD_CHANGEFOLD) && (foldAutomatic & SC_AUTOMATICFOLD_CHANGE)) {
		FoldChanged(mh.line, mh.foldLevelNow, mh.foldLevelPrev);
	}

	// NOW pay the piper WRT "deferred" visual updates
	if (IsLastStep(mh)) {
		SetScrollBars();
		Redraw();
	}

	// If client wants to see this modification
	if (mh.modificationType & modEventMask) {
		if ((mh.modificationType & (SC_MOD_CHANGESTYLE | SC_MOD_CHANGEINDICATOR)) == 0) {
			// Real modification made to text of document.
			NotifyChange();	// Send EN_CHANGE
		}

		SCNotification scn = {};
		scn.nmhdr.code = SCN_MODIFIED;
		scn.position = mh.position;
		scn.modificationType = mh.modificationType;
		scn.text = mh.text;
		scn.length = mh.length;
		scn.linesAdded = mh.linesAdded;
		scn.line = mh.line;
		scn.foldLevelNow = mh.foldLevelNow;
		scn.foldLevelPrev = mh.foldLevelPrev;
		scn.token = mh.token;
		scn.annotationLinesAdded = mh.annotationLinesAdded;
		NotifyParent(scn);
	}
}

void Editor::NotifyDeleted(Document *, void *) {
	/* Do nothing */
}

void Editor::NotifyMacroRecord(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {

	// Enumerates all macroable messages
	switch (iMessage) {
	case SCI_CUT:
	case SCI_COPY:
	case SCI_PASTE:
	case SCI_CLEAR:
	case SCI_REPLACESEL:
	case SCI_ADDTEXT:
	case SCI_INSERTTEXT:
	case SCI_APPENDTEXT:
	case SCI_CLEARALL:
	case SCI_SELECTALL:
	case SCI_GOTOLINE:
	case SCI_GOTOPOS:
	case SCI_SEARCHANCHOR:
	case SCI_SEARCHNEXT:
	case SCI_SEARCHPREV:
	case SCI_LINEDOWN:
	case SCI_LINEDOWNEXTEND:
	case SCI_PARADOWN:
	case SCI_PARADOWNEXTEND:
	case SCI_LINEUP:
	case SCI_LINEUPEXTEND:
	case SCI_PARAUP:
	case SCI_PARAUPEXTEND:
	case SCI_CHARLEFT:
	case SCI_CHARLEFTEXTEND:
	case SCI_CHARRIGHT:
	case SCI_CHARRIGHTEXTEND:
	case SCI_WORDLEFT:
	case SCI_WORDLEFTEXTEND:
	case SCI_WORDRIGHT:
	case SCI_WORDRIGHTEXTEND:
	case SCI_WORDPARTLEFT:
	case SCI_WORDPARTLEFTEXTEND:
	case SCI_WORDPARTRIGHT:
	case SCI_WORDPARTRIGHTEXTEND:
	case SCI_WORDLEFTEND:
	case SCI_WORDLEFTENDEXTEND:
	case SCI_WORDRIGHTEND:
	case SCI_WORDRIGHTENDEXTEND:
	case SCI_HOME:
	case SCI_HOMEEXTEND:
	case SCI_LINEEND:
	case SCI_LINEENDEXTEND:
	case SCI_HOMEWRAP:
	case SCI_HOMEWRAPEXTEND:
	case SCI_LINEENDWRAP:
	case SCI_LINEENDWRAPEXTEND:
	case SCI_DOCUMENTSTART:
	case SCI_DOCUMENTSTARTEXTEND:
	case SCI_DOCUMENTEND:
	case SCI_DOCUMENTENDEXTEND:
	case SCI_STUTTEREDPAGEUP:
	case SCI_STUTTEREDPAGEUPEXTEND:
	case SCI_STUTTEREDPAGEDOWN:
	case SCI_STUTTEREDPAGEDOWNEXTEND:
	case SCI_PAGEUP:
	case SCI_PAGEUPEXTEND:
	case SCI_PAGEDOWN:
	case SCI_PAGEDOWNEXTEND:
	case SCI_EDITTOGGLEOVERTYPE:
	case SCI_CANCEL:
	case SCI_DELETEBACK:
	case SCI_TAB:
	case SCI_BACKTAB:
	case SCI_FORMFEED:
	case SCI_VCHOME:
	case SCI_VCHOMEEXTEND:
	case SCI_VCHOMEWRAP:
	case SCI_VCHOMEWRAPEXTEND:
	case SCI_VCHOMEDISPLAY:
	case SCI_VCHOMEDISPLAYEXTEND:
	case SCI_DELWORDLEFT:
	case SCI_DELWORDRIGHT:
	case SCI_DELWORDRIGHTEND:
	case SCI_DELLINELEFT:
	case SCI_DELLINERIGHT:
	case SCI_LINECOPY:
	case SCI_LINECUT:
	case SCI_LINEDELETE:
	case SCI_LINETRANSPOSE:
	case SCI_LINEDUPLICATE:
	case SCI_LOWERCASE:
	case SCI_UPPERCASE:
	case SCI_LINESCROLLDOWN:
	case SCI_LINESCROLLUP:
	case SCI_DELETEBACKNOTLINE:
	case SCI_HOMEDISPLAY:
	case SCI_HOMEDISPLAYEXTEND:
	case SCI_LINEENDDISPLAY:
	case SCI_LINEENDDISPLAYEXTEND:
	case SCI_SETSELECTIONMODE:
	case SCI_LINEDOWNRECTEXTEND:
	case SCI_LINEUPRECTEXTEND:
	case SCI_CHARLEFTRECTEXTEND:
	case SCI_CHARRIGHTRECTEXTEND:
	case SCI_HOMERECTEXTEND:
	case SCI_VCHOMERECTEXTEND:
	case SCI_LINEENDRECTEXTEND:
	case SCI_PAGEUPRECTEXTEND:
	case SCI_PAGEDOWNRECTEXTEND:
	case SCI_SELECTIONDUPLICATE:
	case SCI_COPYALLOWLINE:
	case SCI_VERTICALCENTRECARET:
	case SCI_MOVESELECTEDLINESUP:
	case SCI_MOVESELECTEDLINESDOWN:
	case SCI_SCROLLTOSTART:
	case SCI_SCROLLTOEND:
		break;

		// Filter out all others like display changes. Also, newlines are redundant
		// with char insert messages.
	case SCI_NEWLINE:
	default:
		//		printf("Filtered out %ld of macro recording\n", iMessage);
		return;
	}

	// Send notification
	SCNotification scn = {};
	scn.nmhdr.code = SCN_MACRORECORD;
	scn.message = iMessage;
	scn.wParam = wParam;
	scn.lParam = lParam;
	NotifyParent(scn);
}

// Something has changed that the container should know about
void Editor::ContainerNeedsUpdate(int flags) {
	needUpdateUI |= flags;
}

/**
 * Force scroll and keep position relative to top of window.
 *
 * If stuttered = true and not already at first/last row, move to first/last row of window.
 * If stuttered = true and already at first/last row, scroll as normal.
 */
void Editor::PageMove(int direction, Selection::selTypes selt, bool stuttered) {
	int topLineNew;
	SelectionPosition newPos;

	int currentLine = pdoc->LineFromPosition(sel.MainCaret());
	int topStutterLine = topLine + caretYSlop;
	int bottomStutterLine =
	    pdoc->LineFromPosition(PositionFromLocation(
	                Point::FromInts(lastXChosen - xOffset, direction * vs.lineHeight * LinesToScroll())))
	    - caretYSlop - 1;

	if (stuttered && (direction < 0 && currentLine > topStutterLine)) {
		topLineNew = topLine;
		newPos = SPositionFromLocation(Point::FromInts(lastXChosen - xOffset, vs.lineHeight * caretYSlop),
			false, false, UserVirtualSpace());

	} else if (stuttered && (direction > 0 && currentLine < bottomStutterLine)) {
		topLineNew = topLine;
		newPos = SPositionFromLocation(Point::FromInts(lastXChosen - xOffset, vs.lineHeight * (LinesToScroll() - caretYSlop)),
			false, false, UserVirtualSpace());

	} else {
		Point pt = LocationFromPosition(sel.MainCaret());

		topLineNew = Platform::Clamp(
		            topLine + direction * LinesToScroll(), 0, MaxScrollPos());
		newPos = SPositionFromLocation(
			Point::FromInts(lastXChosen - xOffset, static_cast<int>(pt.y) + direction * (vs.lineHeight * LinesToScroll())),
			false, false, UserVirtualSpace());
	}

	if (topLineNew != topLine) {
		SetTopLine(topLineNew);
		MovePositionTo(newPos, selt);
		Redraw();
		SetVerticalScrollPos();
	} else {
		MovePositionTo(newPos, selt);
	}
}

void Editor::ChangeCaseOfSelection(int caseMapping) {
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		SelectionRange current = sel.Range(r);
		SelectionRange currentNoVS = current;
		currentNoVS.ClearVirtualSpace();
		size_t rangeBytes = currentNoVS.Length();
		if (rangeBytes > 0) {
			std::string sText = RangeText(currentNoVS.Start().Position(), currentNoVS.End().Position());

			std::string sMapped = CaseMapString(sText, caseMapping);

			if (sMapped != sText) {
				size_t firstDifference = 0;
				while (sMapped[firstDifference] == sText[firstDifference])
					firstDifference++;
				size_t lastDifferenceText = sText.size() - 1;
				size_t lastDifferenceMapped = sMapped.size() - 1;
				while (sMapped[lastDifferenceMapped] == sText[lastDifferenceText]) {
					lastDifferenceText--;
					lastDifferenceMapped--;
				}
				size_t endDifferenceText = sText.size() - 1 - lastDifferenceText;
				pdoc->DeleteChars(
					static_cast<int>(currentNoVS.Start().Position() + firstDifference),
					static_cast<int>(rangeBytes - firstDifference - endDifferenceText));
				const int lengthChange = static_cast<int>(lastDifferenceMapped - firstDifference + 1);
				const int lengthInserted = pdoc->InsertString(
					static_cast<int>(currentNoVS.Start().Position() + firstDifference),
					sMapped.c_str() + firstDifference,
					lengthChange);
				// Automatic movement changes selection so reset to exactly the same as it was.
				int diffSizes = static_cast<int>(sMapped.size() - sText.size()) + lengthInserted - lengthChange;
				if (diffSizes != 0) {
					if (current.anchor > current.caret)
						current.anchor.Add(diffSizes);
					else
						current.caret.Add(diffSizes);
				}
				sel.Range(r) = current;
			}
		}
	}
}

void Editor::LineTranspose() {
	int line = pdoc->LineFromPosition(sel.MainCaret());
	if (line > 0) {
		UndoGroup ug(pdoc);

		const int startPrevious = pdoc->LineStart(line - 1);
		const std::string linePrevious = RangeText(startPrevious, pdoc->LineEnd(line - 1));

		int startCurrent = pdoc->LineStart(line);
		const std::string lineCurrent = RangeText(startCurrent, pdoc->LineEnd(line));

		pdoc->DeleteChars(startCurrent, static_cast<int>(lineCurrent.length()));
		pdoc->DeleteChars(startPrevious, static_cast<int>(linePrevious.length()));
		startCurrent -= static_cast<int>(linePrevious.length());

		startCurrent += pdoc->InsertString(startPrevious, lineCurrent.c_str(),
			static_cast<int>(lineCurrent.length()));
		pdoc->InsertString(startCurrent, linePrevious.c_str(),
			static_cast<int>(linePrevious.length()));
		// Move caret to start of current line
		MovePositionTo(SelectionPosition(startCurrent));
	}
}

void Editor::Duplicate(bool forLine) {
	if (sel.Empty()) {
		forLine = true;
	}
	UndoGroup ug(pdoc);
	const char *eol = "";
	int eolLen = 0;
	if (forLine) {
		eol = StringFromEOLMode(pdoc->eolMode);
		eolLen = istrlen(eol);
	}
	for (size_t r=0; r<sel.Count(); r++) {
		SelectionPosition start = sel.Range(r).Start();
		SelectionPosition end = sel.Range(r).End();
		if (forLine) {
			int line = pdoc->LineFromPosition(sel.Range(r).caret.Position());
			start = SelectionPosition(pdoc->LineStart(line));
			end = SelectionPosition(pdoc->LineEnd(line));
		}
		std::string text = RangeText(start.Position(), end.Position());
		int lengthInserted = eolLen;
		if (forLine)
			lengthInserted = pdoc->InsertString(end.Position(), eol, eolLen);
		pdoc->InsertString(end.Position() + lengthInserted, text.c_str(), static_cast<int>(text.length()));
	}
	if (sel.Count() && sel.IsRectangular()) {
		SelectionPosition last = sel.Last();
		if (forLine) {
			int line = pdoc->LineFromPosition(last.Position());
			last = SelectionPosition(last.Position() + pdoc->LineStart(line+1) - pdoc->LineStart(line));
		}
		if (sel.Rectangular().anchor > sel.Rectangular().caret)
			sel.Rectangular().anchor = last;
		else
			sel.Rectangular().caret = last;
		SetRectangularRange();
	}
}

void Editor::CancelModes() {
	sel.SetMoveExtends(false);
}

void Editor::NewLine() {
	// Remove non-main ranges
	InvalidateSelection(sel.RangeMain(), true);
	sel.SetSelection(sel.RangeMain());
	sel.RangeMain().ClearVirtualSpace();

	// Clear main range and insert line end
	bool needGroupUndo = !sel.Empty();
	if (needGroupUndo)
		pdoc->BeginUndoAction();

	if (!sel.Empty())
		ClearSelection();
	const char *eol = "\n";
	if (pdoc->eolMode == SC_EOL_CRLF) {
		eol = "\r\n";
	} else if (pdoc->eolMode == SC_EOL_CR) {
		eol = "\r";
	} // else SC_EOL_LF -> "\n" already set
	const int insertLength = pdoc->InsertString(sel.MainCaret(), eol, istrlen(eol));
	// Want to end undo group before NotifyChar as applications often modify text here
	if (needGroupUndo)
		pdoc->EndUndoAction();
	if (insertLength > 0) {
		SetEmptySelection(sel.MainCaret() + insertLength);
		while (*eol) {
			NotifyChar(*eol);
			if (recordingMacro) {
				char txt[2];
				txt[0] = *eol;
				txt[1] = '\0';
				NotifyMacroRecord(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(txt));
			}
			eol++;
		}
	}
	SetLastXChosen();
	SetScrollBars();
	EnsureCaretVisible();
	// Avoid blinking during rapid typing:
	ShowCaretAtCurrentPosition();
}

void Editor::CursorUpOrDown(int direction, Selection::selTypes selt) {
	SelectionPosition caretToUse = sel.Range(sel.Main()).caret;
	if (sel.IsRectangular()) {
		if (selt ==  Selection::noSel) {
			caretToUse = (direction > 0) ? sel.Limits().end : sel.Limits().start;
		} else {
			caretToUse = sel.Rectangular().caret;
		}
	}

	Point pt = LocationFromPosition(caretToUse);
	int skipLines = 0;

	if (vs.annotationVisible) {
		int lineDoc = pdoc->LineFromPosition(caretToUse.Position());
		Point ptStartLine = LocationFromPosition(pdoc->LineStart(lineDoc));
		int subLine = static_cast<int>(pt.y - ptStartLine.y) / vs.lineHeight;

		if (direction < 0 && subLine == 0) {
			int lineDisplay = cs.DisplayFromDoc(lineDoc);
			if (lineDisplay > 0) {
				skipLines = pdoc->AnnotationLines(cs.DocFromDisplay(lineDisplay - 1));
			}
		} else if (direction > 0 && subLine >= (cs.GetHeight(lineDoc) - 1 - pdoc->AnnotationLines(lineDoc))) {
			skipLines = pdoc->AnnotationLines(lineDoc);
		}
	}

	int newY = static_cast<int>(pt.y) + (1 + skipLines) * direction * vs.lineHeight;
	SelectionPosition posNew = SPositionFromLocation(
	            Point::FromInts(lastXChosen - xOffset, newY), false, false, UserVirtualSpace());

	if (direction < 0) {
		// Line wrapping may lead to a location on the same line, so
		// seek back if that is the case.
		Point ptNew = LocationFromPosition(posNew.Position());
		while ((posNew.Position() > 0) && (pt.y == ptNew.y)) {
			posNew.Add(-1);
			posNew.SetVirtualSpace(0);
			ptNew = LocationFromPosition(posNew.Position());
		}
	} else if (direction > 0 && posNew.Position() != pdoc->Length()) {
		// There is an equivalent case when moving down which skips
		// over a line.
		Point ptNew = LocationFromPosition(posNew.Position());
		while ((posNew.Position() > caretToUse.Position()) && (ptNew.y > newY)) {
			posNew.Add(-1);
			posNew.SetVirtualSpace(0);
			ptNew = LocationFromPosition(posNew.Position());
		}
	}

	MovePositionTo(MovePositionSoVisible(posNew, direction), selt);
}

void Editor::ParaUpOrDown(int direction, Selection::selTypes selt) {
	int lineDoc, savedPos = sel.MainCaret();
	do {
		MovePositionTo(SelectionPosition(direction > 0 ? pdoc->ParaDown(sel.MainCaret()) : pdoc->ParaUp(sel.MainCaret())), selt);
		lineDoc = pdoc->LineFromPosition(sel.MainCaret());
		if (direction > 0) {
			if (sel.MainCaret() >= pdoc->Length() && !cs.GetVisible(lineDoc)) {
				if (selt == Selection::noSel) {
					MovePositionTo(SelectionPosition(pdoc->LineEndPosition(savedPos)));
				}
				break;
			}
		}
	} while (!cs.GetVisible(lineDoc));
}

int Editor::StartEndDisplayLine(int pos, bool start) {
	RefreshStyleData();
	AutoSurface surface(this);
	int posRet = view.StartEndDisplayLine(surface, *this, pos, start, vs);
	if (posRet == INVALID_POSITION) {
		return pos;
	} else {
		return posRet;
	}
}

int Editor::KeyCommand(unsigned int iMessage) {
	switch (iMessage) {
	case SCI_LINEDOWN:
		CursorUpOrDown(1);
		break;
	case SCI_LINEDOWNEXTEND:
		CursorUpOrDown(1, Selection::selStream);
		break;
	case SCI_LINEDOWNRECTEXTEND:
		CursorUpOrDown(1, Selection::selRectangle);
		break;
	case SCI_PARADOWN:
		ParaUpOrDown(1);
		break;
	case SCI_PARADOWNEXTEND:
		ParaUpOrDown(1, Selection::selStream);
		break;
	case SCI_LINESCROLLDOWN:
		ScrollTo(topLine + 1);
		MoveCaretInsideView(false);
		break;
	case SCI_LINEUP:
		CursorUpOrDown(-1);
		break;
	case SCI_LINEUPEXTEND:
		CursorUpOrDown(-1, Selection::selStream);
		break;
	case SCI_LINEUPRECTEXTEND:
		CursorUpOrDown(-1, Selection::selRectangle);
		break;
	case SCI_PARAUP:
		ParaUpOrDown(-1);
		break;
	case SCI_PARAUPEXTEND:
		ParaUpOrDown(-1, Selection::selStream);
		break;
	case SCI_LINESCROLLUP:
		ScrollTo(topLine - 1);
		MoveCaretInsideView(false);
		break;
	case SCI_CHARLEFT:
		if (SelectionEmpty() || sel.MoveExtends()) {
			if ((sel.Count() == 1) && pdoc->IsLineEndPosition(sel.MainCaret()) && sel.RangeMain().caret.VirtualSpace()) {
				SelectionPosition spCaret = sel.RangeMain().caret;
				spCaret.SetVirtualSpace(spCaret.VirtualSpace() - 1);
				MovePositionTo(spCaret);
			} else if (sel.MoveExtends() && sel.selType == Selection::selStream) {
				MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() - 1), -1));
			} else {
				MovePositionTo(MovePositionSoVisible(
					SelectionPosition((sel.LimitsForRectangularElseMain().start).Position() - 1), -1));
			}
		} else {
			MovePositionTo(sel.LimitsForRectangularElseMain().start);
		}
		SetLastXChosen();
		break;
	case SCI_CHARLEFTEXTEND:
		if (pdoc->IsLineEndPosition(sel.MainCaret()) && sel.RangeMain().caret.VirtualSpace()) {
			SelectionPosition spCaret = sel.RangeMain().caret;
			spCaret.SetVirtualSpace(spCaret.VirtualSpace() - 1);
			MovePositionTo(spCaret, Selection::selStream);
		} else {
			MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() - 1), -1), Selection::selStream);
		}
		SetLastXChosen();
		break;
	case SCI_CHARLEFTRECTEXTEND:
		if (pdoc->IsLineEndPosition(sel.MainCaret()) && sel.RangeMain().caret.VirtualSpace()) {
			SelectionPosition spCaret = sel.RangeMain().caret;
			spCaret.SetVirtualSpace(spCaret.VirtualSpace() - 1);
			MovePositionTo(spCaret, Selection::selRectangle);
		} else {
			MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() - 1), -1), Selection::selRectangle);
		}
		SetLastXChosen();
		break;
	case SCI_CHARRIGHT:
		if (SelectionEmpty() || sel.MoveExtends()) {
			if ((virtualSpaceOptions & SCVS_USERACCESSIBLE) && pdoc->IsLineEndPosition(sel.MainCaret())) {
				SelectionPosition spCaret = sel.RangeMain().caret;
				spCaret.SetVirtualSpace(spCaret.VirtualSpace() + 1);
				MovePositionTo(spCaret);
			} else if (sel.MoveExtends() && sel.selType == Selection::selStream) {
				MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() + 1), 1));
			} else {
				MovePositionTo(MovePositionSoVisible(
					SelectionPosition((sel.LimitsForRectangularElseMain().end).Position() + 1), 1));
			}
		} else {
			MovePositionTo(sel.LimitsForRectangularElseMain().end);
		}
		SetLastXChosen();
		break;
	case SCI_CHARRIGHTEXTEND:
		if ((virtualSpaceOptions & SCVS_USERACCESSIBLE) && pdoc->IsLineEndPosition(sel.MainCaret())) {
			SelectionPosition spCaret = sel.RangeMain().caret;
			spCaret.SetVirtualSpace(spCaret.VirtualSpace() + 1);
			MovePositionTo(spCaret, Selection::selStream);
		} else {
			MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() + 1), 1), Selection::selStream);
		}
		SetLastXChosen();
		break;
	case SCI_CHARRIGHTRECTEXTEND:
		if ((virtualSpaceOptions & SCVS_RECTANGULARSELECTION) && pdoc->IsLineEndPosition(sel.MainCaret())) {
			SelectionPosition spCaret = sel.RangeMain().caret;
			spCaret.SetVirtualSpace(spCaret.VirtualSpace() + 1);
			MovePositionTo(spCaret, Selection::selRectangle);
		} else {
			MovePositionTo(MovePositionSoVisible(SelectionPosition(sel.MainCaret() + 1), 1), Selection::selRectangle);
		}
		SetLastXChosen();
		break;
	case SCI_WORDLEFT:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(sel.MainCaret(), -1), -1));
		SetLastXChosen();
		break;
	case SCI_WORDLEFTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(sel.MainCaret(), -1), -1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_WORDRIGHT:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(sel.MainCaret(), 1), 1));
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordStart(sel.MainCaret(), 1), 1), Selection::selStream);
		SetLastXChosen();
		break;

	case SCI_WORDLEFTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(sel.MainCaret(), -1), -1));
		SetLastXChosen();
		break;
	case SCI_WORDLEFTENDEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(sel.MainCaret(), -1), -1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(sel.MainCaret(), 1), 1));
		SetLastXChosen();
		break;
	case SCI_WORDRIGHTENDEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->NextWordEnd(sel.MainCaret(), 1), 1), Selection::selStream);
		SetLastXChosen();
		break;

	case SCI_HOME:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())));
		SetLastXChosen();
		break;
	case SCI_HOMEEXTEND:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_HOMERECTEXTEND:
		MovePositionTo(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())), Selection::selRectangle);
		SetLastXChosen();
		break;
	case SCI_LINEEND:
		MovePositionTo(pdoc->LineEndPosition(sel.MainCaret()));
		SetLastXChosen();
		break;
	case SCI_LINEENDEXTEND:
		MovePositionTo(pdoc->LineEndPosition(sel.MainCaret()), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_LINEENDRECTEXTEND:
		MovePositionTo(pdoc->LineEndPosition(sel.MainCaret()), Selection::selRectangle);
		SetLastXChosen();
		break;
	case SCI_HOMEWRAP: {
			SelectionPosition homePos = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if (sel.RangeMain().caret <= homePos)
				homePos = SelectionPosition(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())));
			MovePositionTo(homePos);
			SetLastXChosen();
		}
		break;
	case SCI_HOMEWRAPEXTEND: {
			SelectionPosition homePos = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if (sel.RangeMain().caret <= homePos)
				homePos = SelectionPosition(pdoc->LineStart(pdoc->LineFromPosition(sel.MainCaret())));
			MovePositionTo(homePos, Selection::selStream);
			SetLastXChosen();
		}
		break;
	case SCI_LINEENDWRAP: {
			SelectionPosition endPos = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), false), 1);
			SelectionPosition realEndPos = SelectionPosition(pdoc->LineEndPosition(sel.MainCaret()));
			if (endPos > realEndPos      // if moved past visible EOLs
			        || sel.RangeMain().caret >= endPos) // if at end of display line already
				endPos = realEndPos;
			MovePositionTo(endPos);
			SetLastXChosen();
		}
		break;
	case SCI_LINEENDWRAPEXTEND: {
			SelectionPosition endPos = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), false), 1);
			SelectionPosition realEndPos = SelectionPosition(pdoc->LineEndPosition(sel.MainCaret()));
			if (endPos > realEndPos      // if moved past visible EOLs
			        || sel.RangeMain().caret >= endPos) // if at end of display line already
				endPos = realEndPos;
			MovePositionTo(endPos, Selection::selStream);
			SetLastXChosen();
		}
		break;
	case SCI_DOCUMENTSTART:
		MovePositionTo(0);
		SetLastXChosen();
		break;
	case SCI_DOCUMENTSTARTEXTEND:
		MovePositionTo(0, Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_DOCUMENTEND:
		MovePositionTo(pdoc->Length());
		SetLastXChosen();
		break;
	case SCI_DOCUMENTENDEXTEND:
		MovePositionTo(pdoc->Length(), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_STUTTEREDPAGEUP:
		PageMove(-1, Selection::noSel, true);
		break;
	case SCI_STUTTEREDPAGEUPEXTEND:
		PageMove(-1, Selection::selStream, true);
		break;
	case SCI_STUTTEREDPAGEDOWN:
		PageMove(1, Selection::noSel, true);
		break;
	case SCI_STUTTEREDPAGEDOWNEXTEND:
		PageMove(1, Selection::selStream, true);
		break;
	case SCI_PAGEUP:
		PageMove(-1);
		break;
	case SCI_PAGEUPEXTEND:
		PageMove(-1, Selection::selStream);
		break;
	case SCI_PAGEUPRECTEXTEND:
		PageMove(-1, Selection::selRectangle);
		break;
	case SCI_PAGEDOWN:
		PageMove(1);
		break;
	case SCI_PAGEDOWNEXTEND:
		PageMove(1, Selection::selStream);
		break;
	case SCI_PAGEDOWNRECTEXTEND:
		PageMove(1, Selection::selRectangle);
		break;
	case SCI_EDITTOGGLEOVERTYPE:
		inOverstrike = !inOverstrike;
		ShowCaretAtCurrentPosition();
		ContainerNeedsUpdate(SC_UPDATE_CONTENT);
		NotifyUpdateUI();
		break;
	case SCI_CANCEL:            	// Cancel any modes - handled in subclass
		// Also unselect text
		CancelModes();
		break;
	case SCI_DELETEBACK:
		DelCharBack(true);
		if ((caretSticky == SC_CARETSTICKY_OFF) || (caretSticky == SC_CARETSTICKY_WHITESPACE)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case SCI_DELETEBACKNOTLINE:
		DelCharBack(false);
		if ((caretSticky == SC_CARETSTICKY_OFF) || (caretSticky == SC_CARETSTICKY_WHITESPACE)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;
	case SCI_TAB:
		Indent(true);
		if (caretSticky == SC_CARETSTICKY_OFF) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		ShowCaretAtCurrentPosition();		// Avoid blinking
		break;
	case SCI_BACKTAB:
		Indent(false);
		if ((caretSticky == SC_CARETSTICKY_OFF) || (caretSticky == SC_CARETSTICKY_WHITESPACE)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		ShowCaretAtCurrentPosition();		// Avoid blinking
		break;
	case SCI_NEWLINE:
		NewLine();
		break;
	case SCI_FORMFEED:
		AddChar('\f');
		break;
	case SCI_VCHOME:
		MovePositionTo(pdoc->VCHomePosition(sel.MainCaret()));
		SetLastXChosen();
		break;
	case SCI_VCHOMEEXTEND:
		MovePositionTo(pdoc->VCHomePosition(sel.MainCaret()), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_VCHOMERECTEXTEND:
		MovePositionTo(pdoc->VCHomePosition(sel.MainCaret()), Selection::selRectangle);
		SetLastXChosen();
		break;
	case SCI_VCHOMEWRAP: {
			SelectionPosition homePos = SelectionPosition(pdoc->VCHomePosition(sel.MainCaret()));
			SelectionPosition viewLineStart = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if ((viewLineStart < sel.RangeMain().caret) && (viewLineStart > homePos))
				homePos = viewLineStart;

			MovePositionTo(homePos);
			SetLastXChosen();
		}
		break;
	case SCI_VCHOMEWRAPEXTEND: {
			SelectionPosition homePos = SelectionPosition(pdoc->VCHomePosition(sel.MainCaret()));
			SelectionPosition viewLineStart = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if ((viewLineStart < sel.RangeMain().caret) && (viewLineStart > homePos))
				homePos = viewLineStart;

			MovePositionTo(homePos, Selection::selStream);
			SetLastXChosen();
		}
		break;
	case SCI_ZOOMIN:
		if (vs.zoomLevel < 20) {
			vs.zoomLevel++;
			InvalidateStyleRedraw();
			NotifyZoom();
		}
		break;
	case SCI_ZOOMOUT:
		if (vs.zoomLevel > -10) {
			vs.zoomLevel--;
			InvalidateStyleRedraw();
			NotifyZoom();
		}
		break;
	case SCI_DELWORDLEFT: {
			int startWord = pdoc->NextWordStart(sel.MainCaret(), -1);
			pdoc->DeleteChars(startWord, sel.MainCaret() - startWord);
			sel.RangeMain().ClearVirtualSpace();
			SetLastXChosen();
		}
		break;
	case SCI_DELWORDRIGHT: {
			UndoGroup ug(pdoc);
			InvalidateSelection(sel.RangeMain(), true);
			sel.RangeMain().caret = SelectionPosition(
				InsertSpace(sel.RangeMain().caret.Position(), sel.RangeMain().caret.VirtualSpace()));
			sel.RangeMain().anchor = sel.RangeMain().caret;
			int endWord = pdoc->NextWordStart(sel.MainCaret(), 1);
			pdoc->DeleteChars(sel.MainCaret(), endWord - sel.MainCaret());
		}
		break;
	case SCI_DELWORDRIGHTEND: {
			UndoGroup ug(pdoc);
			InvalidateSelection(sel.RangeMain(), true);
			sel.RangeMain().caret = SelectionPosition(
				InsertSpace(sel.RangeMain().caret.Position(), sel.RangeMain().caret.VirtualSpace()));
			int endWord = pdoc->NextWordEnd(sel.MainCaret(), 1);
			pdoc->DeleteChars(sel.MainCaret(), endWord - sel.MainCaret());
		}
		break;
	case SCI_DELLINELEFT: {
			int line = pdoc->LineFromPosition(sel.MainCaret());
			int start = pdoc->LineStart(line);
			pdoc->DeleteChars(start, sel.MainCaret() - start);
			sel.RangeMain().ClearVirtualSpace();
			SetLastXChosen();
		}
		break;
	case SCI_DELLINERIGHT: {
			int line = pdoc->LineFromPosition(sel.MainCaret());
			int end = pdoc->LineEnd(line);
			pdoc->DeleteChars(sel.MainCaret(), end - sel.MainCaret());
		}
		break;
	case SCI_LINECOPY: {
			int lineStart = pdoc->LineFromPosition(SelectionStart().Position());
			int lineEnd = pdoc->LineFromPosition(SelectionEnd().Position());
			CopyRangeToClipboard(pdoc->LineStart(lineStart),
			        pdoc->LineStart(lineEnd + 1));
		}
		break;
	case SCI_LINECUT: {
			int lineStart = pdoc->LineFromPosition(SelectionStart().Position());
			int lineEnd = pdoc->LineFromPosition(SelectionEnd().Position());
			int start = pdoc->LineStart(lineStart);
			int end = pdoc->LineStart(lineEnd + 1);
			SetSelection(start, end);
			Cut();
			SetLastXChosen();
		}
		break;
	case SCI_LINEDELETE: {
			int line = pdoc->LineFromPosition(sel.MainCaret());
			int start = pdoc->LineStart(line);
			int end = pdoc->LineStart(line + 1);
			pdoc->DeleteChars(start, end - start);
		}
		break;
	case SCI_LINETRANSPOSE:
		LineTranspose();
		break;
	case SCI_LINEDUPLICATE:
		Duplicate(true);
		break;
	case SCI_SELECTIONDUPLICATE:
		Duplicate(false);
		break;
	case SCI_LOWERCASE:
		ChangeCaseOfSelection(cmLower);
		break;
	case SCI_UPPERCASE:
		ChangeCaseOfSelection(cmUpper);
		break;
	case SCI_WORDPARTLEFT:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartLeft(sel.MainCaret()), -1));
		SetLastXChosen();
		break;
	case SCI_WORDPARTLEFTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartLeft(sel.MainCaret()), -1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_WORDPARTRIGHT:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartRight(sel.MainCaret()), 1));
		SetLastXChosen();
		break;
	case SCI_WORDPARTRIGHTEXTEND:
		MovePositionTo(MovePositionSoVisible(pdoc->WordPartRight(sel.MainCaret()), 1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_HOMEDISPLAY:
		MovePositionTo(MovePositionSoVisible(
		            StartEndDisplayLine(sel.MainCaret(), true), -1));
		SetLastXChosen();
		break;
	case SCI_VCHOMEDISPLAY: {
			SelectionPosition homePos = SelectionPosition(pdoc->VCHomePosition(sel.MainCaret()));
			SelectionPosition viewLineStart = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if (viewLineStart > homePos)
				homePos = viewLineStart;

			MovePositionTo(homePos);
			SetLastXChosen();
		}
		break;
	case SCI_HOMEDISPLAYEXTEND:
		MovePositionTo(MovePositionSoVisible(
		            StartEndDisplayLine(sel.MainCaret(), true), -1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_VCHOMEDISPLAYEXTEND: {
			SelectionPosition homePos = SelectionPosition(pdoc->VCHomePosition(sel.MainCaret()));
			SelectionPosition viewLineStart = MovePositionSoVisible(StartEndDisplayLine(sel.MainCaret(), true), -1);
			if (viewLineStart > homePos)
				homePos = viewLineStart;

			MovePositionTo(homePos, Selection::selStream);
			SetLastXChosen();
		}
		break;
	case SCI_LINEENDDISPLAY:
		MovePositionTo(MovePositionSoVisible(
		            StartEndDisplayLine(sel.MainCaret(), false), 1));
		SetLastXChosen();
		break;
	case SCI_LINEENDDISPLAYEXTEND:
		MovePositionTo(MovePositionSoVisible(
		            StartEndDisplayLine(sel.MainCaret(), false), 1), Selection::selStream);
		SetLastXChosen();
		break;
	case SCI_SCROLLTOSTART:
		ScrollTo(0);
		break;
	case SCI_SCROLLTOEND:
		ScrollTo(MaxScrollPos());
		break;
	}
	return 0;
}

int Editor::KeyDefault(int, int) {
	return 0;
}

int Editor::KeyDownWithModifiers(int key, int modifiers, bool *consumed) {
	DwellEnd(false);
	int msg = kmap.Find(key, modifiers);
	if (msg) {
		if (consumed)
			*consumed = true;
		return static_cast<int>(WndProc(msg, 0, 0));
	} else {
		if (consumed)
			*consumed = false;
		return KeyDefault(key, modifiers);
	}
}

int Editor::KeyDown(int key, bool shift, bool ctrl, bool alt, bool *consumed) {
	return KeyDownWithModifiers(key, ModifierFlags(shift, ctrl, alt), consumed);
}

void Editor::Indent(bool forwards) {
	UndoGroup ug(pdoc);
	for (size_t r=0; r<sel.Count(); r++) {
		int lineOfAnchor = pdoc->LineFromPosition(sel.Range(r).anchor.Position());
		int caretPosition = sel.Range(r).caret.Position();
		int lineCurrentPos = pdoc->LineFromPosition(caretPosition);
		if (lineOfAnchor == lineCurrentPos) {
			if (forwards) {
				pdoc->DeleteChars(sel.Range(r).Start().Position(), sel.Range(r).Length());
				caretPosition = sel.Range(r).caret.Position();
				if (pdoc->GetColumn(caretPosition) <= pdoc->GetColumn(pdoc->GetLineIndentPosition(lineCurrentPos)) &&
						pdoc->tabIndents) {
					int indentation = pdoc->GetLineIndentation(lineCurrentPos);
					int indentationStep = pdoc->IndentSize();
					const int posSelect = pdoc->SetLineIndentation(
						lineCurrentPos, indentation + indentationStep - indentation % indentationStep);
					sel.Range(r) = SelectionRange(posSelect);
				} else {
					if (pdoc->useTabs) {
						const int lengthInserted = pdoc->InsertString(caretPosition, "\t", 1);
						sel.Range(r) = SelectionRange(caretPosition + lengthInserted);
					} else {
						int numSpaces = (pdoc->tabInChars) -
								(pdoc->GetColumn(caretPosition) % (pdoc->tabInChars));
						if (numSpaces < 1)
							numSpaces = pdoc->tabInChars;
						const std::string spaceText(numSpaces, ' ');
						const int lengthInserted = pdoc->InsertString(caretPosition, spaceText.c_str(),
							static_cast<int>(spaceText.length()));
						sel.Range(r) = SelectionRange(caretPosition + lengthInserted);
					}
				}
			} else {
				if (pdoc->GetColumn(caretPosition) <= pdoc->GetLineIndentation(lineCurrentPos) &&
						pdoc->tabIndents) {
					int indentation = pdoc->GetLineIndentation(lineCurrentPos);
					int indentationStep = pdoc->IndentSize();
					const int posSelect = pdoc->SetLineIndentation(lineCurrentPos, indentation - indentationStep);
					sel.Range(r) = SelectionRange(posSelect);
				} else {
					int newColumn = ((pdoc->GetColumn(caretPosition) - 1) / pdoc->tabInChars) *
							pdoc->tabInChars;
					if (newColumn < 0)
						newColumn = 0;
					int newPos = caretPosition;
					while (pdoc->GetColumn(newPos) > newColumn)
						newPos--;
					sel.Range(r) = SelectionRange(newPos);
				}
			}
		} else {	// Multiline
			int anchorPosOnLine = sel.Range(r).anchor.Position() - pdoc->LineStart(lineOfAnchor);
			int currentPosPosOnLine = caretPosition - pdoc->LineStart(lineCurrentPos);
			// Multiple lines selected so indent / dedent
			int lineTopSel = Platform::Minimum(lineOfAnchor, lineCurrentPos);
			int lineBottomSel = Platform::Maximum(lineOfAnchor, lineCurrentPos);
			if (pdoc->LineStart(lineBottomSel) == sel.Range(r).anchor.Position() || pdoc->LineStart(lineBottomSel) == caretPosition)
				lineBottomSel--;  	// If not selecting any characters on a line, do not indent
			pdoc->Indent(forwards, lineBottomSel, lineTopSel);
			if (lineOfAnchor < lineCurrentPos) {
				if (currentPosPosOnLine == 0)
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor));
				else
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos + 1), pdoc->LineStart(lineOfAnchor));
			} else {
				if (anchorPosOnLine == 0)
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor));
				else
					sel.Range(r) = SelectionRange(pdoc->LineStart(lineCurrentPos), pdoc->LineStart(lineOfAnchor + 1));
			}
		}
	}
	ContainerNeedsUpdate(SC_UPDATE_SELECTION);
}

class CaseFolderASCII : public CaseFolderTable {
public:
	CaseFolderASCII() {
		StandardASCII();
	}
	~CaseFolderASCII() {
	}
};


CaseFolder *Editor::CaseFolderForEncoding() {
	// Simple default that only maps ASCII upper case to lower case.
	return new CaseFolderASCII();
}

/**
 * Search of a text in the document, in the given range.
 * @return The position of the found text, -1 if not found.
 */
long Editor::FindText(
    uptr_t wParam,		///< Search modes : @c SCFIND_MATCHCASE, @c SCFIND_WHOLEWORD,
    ///< @c SCFIND_WORDSTART, @c SCFIND_REGEXP or @c SCFIND_POSIX.
    sptr_t lParam) {	///< @c TextToFind structure: The text to search for in the given range.

	Sci_TextToFind *ft = reinterpret_cast<Sci_TextToFind *>(lParam);
	int lengthFound = istrlen(ft->lpstrText);
	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	try {
		long pos = pdoc->FindText(
			static_cast<int>(ft->chrg.cpMin),
			static_cast<int>(ft->chrg.cpMax),
			ft->lpstrText,
			(wParam & SCFIND_MATCHCASE) != 0,
			(wParam & SCFIND_WHOLEWORD) != 0,
			(wParam & SCFIND_WORDSTART) != 0,
			(wParam & SCFIND_REGEXP) != 0,
			static_cast<int>(wParam),
			&lengthFound);
		if (pos != -1) {
			ft->chrgText.cpMin = pos;
			ft->chrgText.cpMax = pos + lengthFound;
		}
		return static_cast<int>(pos);
	} catch (RegexError &) {
		errorStatus = SC_STATUS_WARN_REGEX;
		return -1;
	}
}

/**
 * Relocatable search support : Searches relative to current selection
 * point and sets the selection to the found text range with
 * each search.
 */
/**
 * Anchor following searches at current selection start: This allows
 * multiple incremental interactive searches to be macro recorded
 * while still setting the selection to found text so the find/select
 * operation is self-contained.
 */
void Editor::SearchAnchor() {
	searchAnchor = SelectionStart().Position();
}

/**
 * Find text from current search anchor: Must call @c SearchAnchor first.
 * Used for next text and previous text requests.
 * @return The position of the found text, -1 if not found.
 */
long Editor::SearchText(
    unsigned int iMessage,		///< Accepts both @c SCI_SEARCHNEXT and @c SCI_SEARCHPREV.
    uptr_t wParam,				///< Search modes : @c SCFIND_MATCHCASE, @c SCFIND_WHOLEWORD,
    ///< @c SCFIND_WORDSTART, @c SCFIND_REGEXP or @c SCFIND_POSIX.
    sptr_t lParam) {			///< The text to search for.

	const char *txt = reinterpret_cast<char *>(lParam);
	long pos;
	int lengthFound = istrlen(txt);
	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	try {
		if (iMessage == SCI_SEARCHNEXT) {
			pos = pdoc->FindText(searchAnchor, pdoc->Length(), txt,
					(wParam & SCFIND_MATCHCASE) != 0,
					(wParam & SCFIND_WHOLEWORD) != 0,
					(wParam & SCFIND_WORDSTART) != 0,
					(wParam & SCFIND_REGEXP) != 0,
					static_cast<int>(wParam),
					&lengthFound);
		} else {
			pos = pdoc->FindText(searchAnchor, 0, txt,
					(wParam & SCFIND_MATCHCASE) != 0,
					(wParam & SCFIND_WHOLEWORD) != 0,
					(wParam & SCFIND_WORDSTART) != 0,
					(wParam & SCFIND_REGEXP) != 0,
					static_cast<int>(wParam),
					&lengthFound);
		}
	} catch (RegexError &) {
		errorStatus = SC_STATUS_WARN_REGEX;
		return -1;
	}
	if (pos != -1) {
		SetSelection(static_cast<int>(pos), static_cast<int>(pos + lengthFound));
	}

	return pos;
}

std::string Editor::CaseMapString(const std::string &s, int caseMapping) {
	std::string ret(s);
	for (size_t i=0; i<ret.size(); i++) {
		switch (caseMapping) {
			case cmUpper:
				if (ret[i] >= 'a' && ret[i] <= 'z')
					ret[i] = static_cast<char>(ret[i] - 'a' + 'A');
				break;
			case cmLower:
				if (ret[i] >= 'A' && ret[i] <= 'Z')
					ret[i] = static_cast<char>(ret[i] - 'A' + 'a');
				break;
		}
	}
	return ret;
}

/**
 * Search for text in the target range of the document.
 * @return The position of the found text, -1 if not found.
 */
long Editor::SearchInTarget(const char *text, int length) {
	int lengthFound = length;

	if (!pdoc->HasCaseFolder())
		pdoc->SetCaseFolder(CaseFolderForEncoding());
	try {
		long pos = pdoc->FindText(targetStart, targetEnd, text,
				(searchFlags & SCFIND_MATCHCASE) != 0,
				(searchFlags & SCFIND_WHOLEWORD) != 0,
				(searchFlags & SCFIND_WORDSTART) != 0,
				(searchFlags & SCFIND_REGEXP) != 0,
				searchFlags,
				&lengthFound);
		if (pos != -1) {
			targetStart = static_cast<int>(pos);
			targetEnd = static_cast<int>(pos + lengthFound);
		}
		return pos;
	} catch (RegexError &) {
		errorStatus = SC_STATUS_WARN_REGEX;
		return -1;
	}
}

void Editor::GoToLine(int lineNo) {
	if (lineNo > pdoc->LinesTotal())
		lineNo = pdoc->LinesTotal();
	if (lineNo < 0)
		lineNo = 0;
	SetEmptySelection(pdoc->LineStart(lineNo));
	ShowCaretAtCurrentPosition();
	EnsureCaretVisible();
}

static bool Close(Point pt1, Point pt2, Point threshold) {
	if (std::abs(pt1.x - pt2.x) > threshold.x)
		return false;
	if (std::abs(pt1.y - pt2.y) > threshold.y)
		return false;
	return true;
}

std::string Editor::RangeText(int start, int end) const {
	if (start < end) {
		int len = end - start;
		std::string ret(len, '\0');
		for (int i = 0; i < len; i++) {
			ret[i] = pdoc->CharAt(start + i);
		}
		return ret;
	}
	return std::string();
}

void Editor::CopySelectionRange(SelectionText *ss, bool allowLineCopy) {
	if (sel.Empty()) {
		if (allowLineCopy) {
			int currentLine = pdoc->LineFromPosition(sel.MainCaret());
			int start = pdoc->LineStart(currentLine);
			int end = pdoc->LineEnd(currentLine);

			std::string text = RangeText(start, end);
			if (pdoc->eolMode != SC_EOL_LF)
				text.push_back('\r');
			if (pdoc->eolMode != SC_EOL_CR)
				text.push_back('\n');
			ss->Copy(text, pdoc->dbcsCodePage,
				vs.styles[STYLE_DEFAULT].characterSet, false, true);
		}
	} else {
		std::string text;
		std::vector<SelectionRange> rangesInOrder = sel.RangesCopy();
		if (sel.selType == Selection::selRectangle)
			std::sort(rangesInOrder.begin(), rangesInOrder.end());
		for (size_t r=0; r<rangesInOrder.size(); r++) {
			SelectionRange current = rangesInOrder[r];
			text.append(RangeText(current.Start().Position(), current.End().Position()));
			if (sel.selType == Selection::selRectangle) {
				if (pdoc->eolMode != SC_EOL_LF)
					text.push_back('\r');
				if (pdoc->eolMode != SC_EOL_CR)
					text.push_back('\n');
			}
		}
		ss->Copy(text, pdoc->dbcsCodePage,
			vs.styles[STYLE_DEFAULT].characterSet, sel.IsRectangular(), sel.selType == Selection::selLines);
	}
}

void Editor::CopyRangeToClipboard(int start, int end) {
	start = pdoc->ClampPositionIntoDocument(start);
	end = pdoc->ClampPositionIntoDocument(end);
	SelectionText selectedText;
	std::string text = RangeText(start, end);
	selectedText.Copy(text,
		pdoc->dbcsCodePage, vs.styles[STYLE_DEFAULT].characterSet, false, false);
	CopyToClipboard(selectedText);
}

void Editor::CopyText(int length, const char *text) {
	SelectionText selectedText;
	selectedText.Copy(std::string(text, length),
		pdoc->dbcsCodePage, vs.styles[STYLE_DEFAULT].characterSet, false, false);
	CopyToClipboard(selectedText);
}

void Editor::SetDragPosition(SelectionPosition newPos) {
	if (newPos.Position() >= 0) {
		newPos = MovePositionOutsideChar(newPos, 1);
		posDrop = newPos;
	}
	if (!(posDrag == newPos)) {
		caret.on = true;
		if (FineTickerAvailable()) {
			FineTickerCancel(tickCaret);
			if ((caret.active) && (caret.period > 0) && (newPos.Position() < 0))
				FineTickerStart(tickCaret, caret.period, caret.period/10);
		} else {
			SetTicking(true);
		}
		InvalidateCaret();
		posDrag = newPos;
		InvalidateCaret();
	}
}

void Editor::DisplayCursor(Window::Cursor c) {
	if (cursorMode == SC_CURSORNORMAL)
		wMain.SetCursor(c);
	else
		wMain.SetCursor(static_cast<Window::Cursor>(cursorMode));
}

bool Editor::DragThreshold(Point ptStart, Point ptNow) {
	int xMove = static_cast<int>(ptStart.x - ptNow.x);
	int yMove = static_cast<int>(ptStart.y - ptNow.y);
	int distanceSquared = xMove * xMove + yMove * yMove;
	return distanceSquared > 16;
}

void Editor::StartDrag() {
	// Always handled by subclasses
	//SetMouseCapture(true);
	//DisplayCursor(Window::cursorArrow);
}

void Editor::DropAt(SelectionPosition position, const char *value, size_t lengthValue, bool moving, bool rectangular) {
	//Platform::DebugPrintf("DropAt %d %d\n", inDragDrop, position);
	if (inDragDrop == ddDragging)
		dropWentOutside = false;

	bool positionWasInSelection = PositionInSelection(position.Position());

	bool positionOnEdgeOfSelection =
	    (position == SelectionStart()) || (position == SelectionEnd());

	if ((inDragDrop != ddDragging) || !(positionWasInSelection) ||
	        (positionOnEdgeOfSelection && !moving)) {

		SelectionPosition selStart = SelectionStart();
		SelectionPosition selEnd = SelectionEnd();

		UndoGroup ug(pdoc);

		SelectionPosition positionAfterDeletion = position;
		if ((inDragDrop == ddDragging) && moving) {
			// Remove dragged out text
			if (rectangular || sel.selType == Selection::selLines) {
				for (size_t r=0; r<sel.Count(); r++) {
					if (position >= sel.Range(r).Start()) {
						if (position > sel.Range(r).End()) {
							positionAfterDeletion.Add(-sel.Range(r).Length());
						} else {
							positionAfterDeletion.Add(-SelectionRange(position, sel.Range(r).Start()).Length());
						}
					}
				}
			} else {
				if (position > selStart) {
					positionAfterDeletion.Add(-SelectionRange(selEnd, selStart).Length());
				}
			}
			ClearSelection();
		}
		position = positionAfterDeletion;

		std::string convertedText = Document::TransformLineEnds(value, lengthValue, pdoc->eolMode);

		if (rectangular) {
			PasteRectangular(position, convertedText.c_str(), static_cast<int>(convertedText.length()));
			// Should try to select new rectangle but it may not be a rectangle now so just select the drop position
			SetEmptySelection(position);
		} else {
			position = MovePositionOutsideChar(position, sel.MainCaret() - position.Position());
			position = SelectionPosition(InsertSpace(position.Position(), position.VirtualSpace()));
			const int lengthInserted = pdoc->InsertString(
				position.Position(), convertedText.c_str(), static_cast<int>(convertedText.length()));
			if (lengthInserted > 0) {
				SelectionPosition posAfterInsertion = position;
				posAfterInsertion.Add(lengthInserted);
				SetSelection(posAfterInsertion, position);
			}
		}
	} else if (inDragDrop == ddDragging) {
		SetEmptySelection(position);
	}
}

void Editor::DropAt(SelectionPosition position, const char *value, bool moving, bool rectangular) {
	DropAt(position, value, strlen(value), moving, rectangular);
}

/**
 * @return true if given position is inside the selection,
 */
bool Editor::PositionInSelection(int pos) {
	pos = MovePositionOutsideChar(pos, sel.MainCaret() - pos);
	for (size_t r=0; r<sel.Count(); r++) {
		if (sel.Range(r).Contains(pos))
			return true;
	}
	return false;
}

bool Editor::PointInSelection(Point pt) {
	SelectionPosition pos = SPositionFromLocation(pt, false, true);
	Point ptPos = LocationFromPosition(pos);
	for (size_t r=0; r<sel.Count(); r++) {
		SelectionRange range = sel.Range(r);
		if (range.Contains(pos)) {
			bool hit = true;
			if (pos == range.Start()) {
				// see if just before selection
				if (pt.x < ptPos.x) {
					hit = false;
				}
			}
			if (pos == range.End()) {
				// see if just after selection
				if (pt.x > ptPos.x) {
					hit = false;
				}
			}
			if (hit)
				return true;
		}
	}
	return false;
}

bool Editor::PointInSelMargin(Point pt) const {
	// Really means: "Point in a margin"
	if (vs.fixedColumnWidth > 0) {	// There is a margin
		PRectangle rcSelMargin = GetClientRectangle();
		rcSelMargin.right = static_cast<XYPOSITION>(vs.textStart - vs.leftMarginWidth);
		rcSelMargin.left = static_cast<XYPOSITION>(vs.textStart - vs.fixedColumnWidth);
		return rcSelMargin.ContainsWholePixel(pt);
	} else {
		return false;
	}
}

Window::Cursor Editor::GetMarginCursor(Point pt) const {
	int x = 0;
	for (int margin = 0; margin <= SC_MAX_MARGIN; margin++) {
		if ((pt.x >= x) && (pt.x < x + vs.ms[margin].width))
			return static_cast<Window::Cursor>(vs.ms[margin].cursor);
		x += vs.ms[margin].width;
	}
	return Window::cursorReverseArrow;
}

void Editor::TrimAndSetSelection(int currentPos_, int anchor_) {
	sel.TrimSelection(SelectionRange(currentPos_, anchor_));
	SetSelection(currentPos_, anchor_);
}

void Editor::LineSelection(int lineCurrentPos_, int lineAnchorPos_, bool wholeLine) {
	int selCurrentPos, selAnchorPos;
	if (wholeLine) {
		int lineCurrent_ = pdoc->LineFromPosition(lineCurrentPos_);
		int lineAnchor_ = pdoc->LineFromPosition(lineAnchorPos_);
		if (lineAnchorPos_ < lineCurrentPos_) {
			selCurrentPos = pdoc->LineStart(lineCurrent_ + 1);
			selAnchorPos = pdoc->LineStart(lineAnchor_);
		} else if (lineAnchorPos_ > lineCurrentPos_) {
			selCurrentPos = pdoc->LineStart(lineCurrent_);
			selAnchorPos = pdoc->LineStart(lineAnchor_ + 1);
		} else { // Same line, select it
			selCurrentPos = pdoc->LineStart(lineAnchor_ + 1);
			selAnchorPos = pdoc->LineStart(lineAnchor_);
		}
	} else {
		if (lineAnchorPos_ < lineCurrentPos_) {
			selCurrentPos = StartEndDisplayLine(lineCurrentPos_, false) + 1;
			selCurrentPos = pdoc->MovePositionOutsideChar(selCurrentPos, 1);
			selAnchorPos = StartEndDisplayLine(lineAnchorPos_, true);
		} else if (lineAnchorPos_ > lineCurrentPos_) {
			selCurrentPos = StartEndDisplayLine(lineCurrentPos_, true);
			selAnchorPos = StartEndDisplayLine(lineAnchorPos_, false) + 1;
			selAnchorPos = pdoc->MovePositionOutsideChar(selAnchorPos, 1);
		} else { // Same line, select it
			selCurrentPos = StartEndDisplayLine(lineAnchorPos_, false) + 1;
			selCurrentPos = pdoc->MovePositionOutsideChar(selCurrentPos, 1);
			selAnchorPos = StartEndDisplayLine(lineAnchorPos_, true);
		}
	}
	TrimAndSetSelection(selCurrentPos, selAnchorPos);
}

void Editor::WordSelection(int pos) {
	if (pos < wordSelectAnchorStartPos) {
		// Extend backward to the word containing pos.
		// Skip ExtendWordSelect if the line is empty or if pos is after the last character.
		// This ensures that a series of empty lines isn't counted as a single "word".
		if (!pdoc->IsLineEndPosition(pos))
			pos = pdoc->ExtendWordSelect(pdoc->MovePositionOutsideChar(pos + 1, 1), -1);
		TrimAndSetSelection(pos, wordSelectAnchorEndPos);
	} else if (pos > wordSelectAnchorEndPos) {
		// Extend forward to the word containing the character to the left of pos.
		// Skip ExtendWordSelect if the line is empty or if pos is the first position on the line.
		// This ensures that a series of empty lines isn't counted as a single "word".
		if (pos > pdoc->LineStart(pdoc->LineFromPosition(pos)))
			pos = pdoc->ExtendWordSelect(pdoc->MovePositionOutsideChar(pos - 1, -1), 1);
		TrimAndSetSelection(pos, wordSelectAnchorStartPos);
	} else {
		// Select only the anchored word
		if (pos >= originalAnchorPos)
			TrimAndSetSelection(wordSelectAnchorEndPos, wordSelectAnchorStartPos);
		else
			TrimAndSetSelection(wordSelectAnchorStartPos, wordSelectAnchorEndPos);
	}
}

void Editor::DwellEnd(bool mouseMoved) {
	if (mouseMoved)
		ticksToDwell = dwellDelay;
	else
		ticksToDwell = SC_TIME_FOREVER;
	if (dwelling && (dwellDelay < SC_TIME_FOREVER)) {
		dwelling = false;
		NotifyDwelling(ptMouseLast, dwelling);
	}
	if (FineTickerAvailable()) {
		FineTickerCancel(tickDwell);
		if (mouseMoved && (dwellDelay < SC_TIME_FOREVER)) {
			//FineTickerStart(tickDwell, dwellDelay, dwellDelay/10);
		}
	}
}

void Editor::MouseLeave() {
	SetHotSpotRange(NULL);
	if (!HaveMouseCapture()) {
		ptMouseLast = Point(-1,-1);
		DwellEnd(true);
	}
}

static bool AllowVirtualSpace(int virtualSpaceOptions, bool rectangular) {
	return (!rectangular && ((virtualSpaceOptions & SCVS_USERACCESSIBLE) != 0))
		|| (rectangular && ((virtualSpaceOptions & SCVS_RECTANGULARSELECTION) != 0));
}

void Editor::ButtonDownWithModifiers(Point pt, unsigned int curTime, int modifiers) {
	SetHoverIndicatorPoint(pt);
	//Platform::DebugPrintf("ButtonDown %d %d = %d alt=%d %d\n", curTime, lastClickTime, curTime - lastClickTime, alt, inDragDrop);
	ptMouseLast = pt;
	const bool ctrl = (modifiers & SCI_CTRL) != 0;
	const bool shift = (modifiers & SCI_SHIFT) != 0;
	const bool alt = (modifiers & SCI_ALT) != 0;
	SelectionPosition newPos = SPositionFromLocation(pt, false, false, AllowVirtualSpace(virtualSpaceOptions, alt));
	newPos = MovePositionOutsideChar(newPos, sel.MainCaret() - newPos.Position());
	SelectionPosition newCharPos = SPositionFromLocation(pt, false, true, false);
	newCharPos = MovePositionOutsideChar(newCharPos, -1);
	inDragDrop = ddNone;
	sel.SetMoveExtends(false);

	if (NotifyMarginClick(pt, modifiers))
		return;

	NotifyIndicatorClick(true, newPos.Position(), modifiers);

	bool inSelMargin = PointInSelMargin(pt);
	// In margin ctrl+(double)click should always select everything
	if (ctrl && inSelMargin) {
		SelectAll();
		lastClickTime = curTime;
		lastClick = pt;
		return;
	}
	if (shift && !inSelMargin) {
		SetSelection(newPos);
	}
	if (((curTime - lastClickTime) < Platform::DoubleClickTime()) && Close(pt, lastClick, doubleClickCloseThreshold)) {
		//Platform::DebugPrintf("Double click %d %d = %d\n", curTime, lastClickTime, curTime - lastClickTime);
		SetMouseCapture(true);
		if (FineTickerAvailable()) {
			FineTickerStart(tickScroll, 100, 10);
		}
		if (!ctrl || !multipleSelection || (selectionType != selChar && selectionType != selWord))
			SetEmptySelection(newPos.Position());
		bool doubleClick = false;
		// Stop mouse button bounce changing selection type
		if (!Platform::MouseButtonBounce() || curTime != lastClickTime) {
			if (inSelMargin) {
				// Inside margin selection type should be either selSubLine or selWholeLine.
				if (selectionType == selSubLine) {
					// If it is selSubLine, we're inside a *double* click and word wrap is enabled,
					// so we switch to selWholeLine in order to select whole line.
					selectionType = selWholeLine;
				} else if (selectionType != selSubLine && selectionType != selWholeLine) {
					// If it is neither, reset selection type to line selection.
					selectionType = (Wrapping() && (marginOptions & SC_MARGINOPTION_SUBLINESELECT)) ? selSubLine : selWholeLine;
				}
			} else {
				if (selectionType == selChar) {
					selectionType = selWord;
					doubleClick = true;
				} else if (selectionType == selWord) {
					// Since we ended up here, we're inside a *triple* click, which should always select
					// whole line regardless of word wrap being enabled or not.
					selectionType = selWholeLine;
				} else {
					selectionType = selChar;
					originalAnchorPos = sel.MainCaret();
				}
			}
		}

		if (selectionType == selWord) {
			int charPos = originalAnchorPos;
			if (sel.MainCaret() == originalAnchorPos) {
				charPos = PositionFromLocation(pt, false, true);
				charPos = MovePositionOutsideChar(charPos, -1);
			}

			int startWord, endWord;
			if ((sel.MainCaret() >= originalAnchorPos) && !pdoc->IsLineEndPosition(charPos)) {
				startWord = pdoc->ExtendWordSelect(pdoc->MovePositionOutsideChar(charPos + 1, 1), -1);
				endWord = pdoc->ExtendWordSelect(charPos, 1);
			} else {
				// Selecting backwards, or anchor beyond last character on line. In these cases,
				// we select the word containing the character to the *left* of the anchor.
				if (charPos > pdoc->LineStart(pdoc->LineFromPosition(charPos))) {
					startWord = pdoc->ExtendWordSelect(charPos, -1);
					endWord = pdoc->ExtendWordSelect(startWord, 1);
				} else {
					// Anchor at start of line; select nothing to begin with.
					startWord = charPos;
					endWord = charPos;
				}
			}

			wordSelectAnchorStartPos = startWord;
			wordSelectAnchorEndPos = endWord;
			wordSelectInitialCaretPos = sel.MainCaret();
			WordSelection(wordSelectInitialCaretPos);
		} else if (selectionType == selSubLine || selectionType == selWholeLine) {
			lineAnchorPos = newPos.Position();
			LineSelection(lineAnchorPos, lineAnchorPos, selectionType == selWholeLine);
			//Platform::DebugPrintf("Triple click: %d - %d\n", anchor, currentPos);
		} else {
			SetEmptySelection(sel.MainCaret());
		}
		//Platform::DebugPrintf("Double click: %d - %d\n", anchor, currentPos);
		if (doubleClick) {
			NotifyDoubleClick(pt, modifiers);
			if (PositionIsHotspot(newCharPos.Position()))
				NotifyHotSpotDoubleClicked(newCharPos.Position(), modifiers);
		}
	} else {	// Single click
		if (inSelMargin) {
			sel.selType = Selection::selStream;
			if (!shift) {
				// Single click in margin: select whole line or only subline if word wrap is enabled
				lineAnchorPos = newPos.Position();
				selectionType = (Wrapping() && (marginOptions & SC_MARGINOPTION_SUBLINESELECT)) ? selSubLine : selWholeLine;
				LineSelection(lineAnchorPos, lineAnchorPos, selectionType == selWholeLine);
			} else {
				// Single shift+click in margin: select from line anchor to clicked line
				if (sel.MainAnchor() > sel.MainCaret())
					lineAnchorPos = sel.MainAnchor() - 1;
				else
					lineAnchorPos = sel.MainAnchor();
				// Reset selection type if there is an empty selection.
				// This ensures that we don't end up stuck in previous selection mode, which is no longer valid.
				// Otherwise, if there's a non empty selection, reset selection type only if it differs from selSubLine and selWholeLine.
				// This ensures that we continue selecting in the same selection mode.
				if (sel.Empty() || (selectionType != selSubLine && selectionType != selWholeLine))
					selectionType = (Wrapping() && (marginOptions & SC_MARGINOPTION_SUBLINESELECT)) ? selSubLine : selWholeLine;
				LineSelection(newPos.Position(), lineAnchorPos, selectionType == selWholeLine);
			}

			SetDragPosition(SelectionPosition(invalidPosition));
			SetMouseCapture(true);
			if (FineTickerAvailable()) {
				FineTickerStart(tickScroll, 100, 10);
			}
		} else {
			if (PointIsHotspot(pt)) {
				NotifyHotSpotClicked(newCharPos.Position(), modifiers);
				hotSpotClickPos = newCharPos.Position();
			}
			if (!shift) {
				if (PointInSelection(pt) && !SelectionEmpty())
					inDragDrop = ddInitial;
				else
					inDragDrop = ddNone;
			}
			SetMouseCapture(true);
			if (FineTickerAvailable()) {
				FineTickerStart(tickScroll, 100, 10);
			}
			if (inDragDrop != ddInitial) {
				SetDragPosition(SelectionPosition(invalidPosition));
				if (!shift) {
					if (ctrl && multipleSelection) {
						SelectionRange range(newPos);
						sel.TentativeSelection(range);
						InvalidateSelection(range, true);
					} else {
						InvalidateSelection(SelectionRange(newPos), true);
						if (sel.Count() > 1)
							Redraw();
						if ((sel.Count() > 1) || (sel.selType != Selection::selStream))
							sel.Clear();
						sel.selType = alt ? Selection::selRectangle : Selection::selStream;
						SetSelection(newPos, newPos);
					}
				}
				SelectionPosition anchorCurrent = newPos;
				if (shift)
					anchorCurrent = sel.IsRectangular() ?
						sel.Rectangular().anchor : sel.RangeMain().anchor;
				sel.selType = alt ? Selection::selRectangle : Selection::selStream;
				selectionType = selChar;
				originalAnchorPos = sel.MainCaret();
				sel.Rectangular() = SelectionRange(newPos, anchorCurrent);
				SetRectangularRange();
			}
		}
	}
	lastClickTime = curTime;
	lastClick = pt;
	lastXChosen = static_cast<int>(pt.x) + xOffset;
	ShowCaretAtCurrentPosition();
}

void Editor::ButtonDown(Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) {
	return ButtonDownWithModifiers(pt, curTime, ModifierFlags(shift, ctrl, alt));
}

bool Editor::PositionIsHotspot(int position) const {
	return vs.styles[static_cast<unsigned char>(pdoc->StyleAt(position))].hotspot;
}

bool Editor::PointIsHotspot(Point pt) {
	int pos = PositionFromLocation(pt, true, true);
	if (pos == INVALID_POSITION)
		return false;
	return PositionIsHotspot(pos);
}

void Editor::SetHoverIndicatorPosition(int position) {
	int hoverIndicatorPosPrev = hoverIndicatorPos;
	hoverIndicatorPos = INVALID_POSITION;
	if (vs.indicatorsDynamic == 0)
		return;
	if (position != INVALID_POSITION) {
		for (Decoration *deco = pdoc->decorations.root; deco; deco = deco->next) {
			if (vs.indicators[deco->indicator].IsDynamic()) {
				if (pdoc->decorations.ValueAt(deco->indicator, position)) {
					hoverIndicatorPos = position;
				}
			}
		}
	}
	if (hoverIndicatorPosPrev != hoverIndicatorPos) {
		if (hoverIndicatorPosPrev != INVALID_POSITION)
			InvalidateRange(hoverIndicatorPosPrev, hoverIndicatorPosPrev + 1);
		if (hoverIndicatorPos != INVALID_POSITION)
			InvalidateRange(hoverIndicatorPos, hoverIndicatorPos + 1);
	}
}

void Editor::SetHoverIndicatorPoint(Point pt) {
	if (vs.indicatorsDynamic == 0) {
		SetHoverIndicatorPosition(INVALID_POSITION);
	} else {
		SetHoverIndicatorPosition(PositionFromLocation(pt, true, true));
	}
}

void Editor::SetHotSpotRange(Point *pt) {
	if (pt) {
		int pos = PositionFromLocation(*pt, false, true);

		// If we don't limit this to word characters then the
		// range can encompass more than the run range and then
		// the underline will not be drawn properly.
		Range hsNew;
		hsNew.start = pdoc->ExtendStyleRange(pos, -1, vs.hotspotSingleLine);
		hsNew.end = pdoc->ExtendStyleRange(pos, 1, vs.hotspotSingleLine);

		// Only invalidate the range if the hotspot range has changed...
		if (!(hsNew == hotspot)) {
			if (hotspot.Valid()) {
				InvalidateRange(hotspot.start, hotspot.end);
			}
			hotspot = hsNew;
			InvalidateRange(hotspot.start, hotspot.end);
		}
	} else {
		if (hotspot.Valid()) {
			InvalidateRange(hotspot.start, hotspot.end);
		}
		hotspot = Range(invalidPosition);
	}
}

Range Editor::GetHotSpotRange() const {
	return hotspot;
}

void Editor::ButtonMoveWithModifiers(Point pt, int modifiers) {
	if ((ptMouseLast.x != pt.x) || (ptMouseLast.y != pt.y)) {
		DwellEnd(true);
	}

	SelectionPosition movePos = SPositionFromLocation(pt, false, false,
		AllowVirtualSpace(virtualSpaceOptions, sel.IsRectangular()));
	movePos = MovePositionOutsideChar(movePos, sel.MainCaret() - movePos.Position());

	if (inDragDrop == ddInitial) {
		if (DragThreshold(ptMouseLast, pt)) {
			SetMouseCapture(false);
			if (FineTickerAvailable()) {
				FineTickerCancel(tickScroll);
			}
			SetDragPosition(movePos);
			CopySelectionRange(&drag);
			StartDrag();
		}
		return;
	}

	ptMouseLast = pt;
	PRectangle rcClient = GetClientRectangle();
	Point ptOrigin = GetVisibleOriginInMain();
	rcClient.Move(0, -ptOrigin.y);
	if (FineTickerAvailable() && (dwellDelay < SC_TIME_FOREVER) && rcClient.Contains(pt)) {
		FineTickerStart(tickDwell, dwellDelay, dwellDelay/10);
	}
	//Platform::DebugPrintf("Move %d %d\n", pt.x, pt.y);
	if (HaveMouseCapture()) {

		// Slow down autoscrolling/selection
		autoScrollTimer.ticksToWait -= timer.tickSize;
		if (autoScrollTimer.ticksToWait > 0)
			return;
		autoScrollTimer.ticksToWait = autoScrollDelay;

		// Adjust selection
		if (posDrag.IsValid()) {
			SetDragPosition(movePos);
		} else {
			if (selectionType == selChar) {
				if (sel.selType == Selection::selStream && (modifiers & SCI_ALT) && mouseSelectionRectangularSwitch) {
					sel.selType = Selection::selRectangle;
				}
				if (sel.IsRectangular()) {
					sel.Rectangular() = SelectionRange(movePos, sel.Rectangular().anchor);
					SetSelection(movePos, sel.RangeMain().anchor);
				} else if (sel.Count() > 1) {
					InvalidateSelection(sel.RangeMain(), false);
					SelectionRange range(movePos, sel.RangeMain().anchor);
					sel.TentativeSelection(range);
					InvalidateSelection(range, true);
				} else {
					SetSelection(movePos, sel.RangeMain().anchor);
				}
			} else if (selectionType == selWord) {
				// Continue selecting by word
				if (movePos.Position() == wordSelectInitialCaretPos) {  // Didn't move
					// No need to do anything. Previously this case was lumped
					// in with "Moved forward", but that can be harmful in this
					// case: a handler for the NotifyDoubleClick re-adjusts
					// the selection for a fancier definition of "word" (for
					// example, in Perl it is useful to include the leading
					// '$', '%' or '@' on variables for word selection). In this
					// the ButtonMove() called via Tick() for auto-scrolling
					// could result in the fancier word selection adjustment
					// being unmade.
				} else {
					wordSelectInitialCaretPos = -1;
					WordSelection(movePos.Position());
				}
			} else {
				// Continue selecting by line
				LineSelection(movePos.Position(), lineAnchorPos, selectionType == selWholeLine);
			}
		}

		// Autoscroll
		int lineMove = DisplayFromPosition(movePos.Position());
		if (pt.y > rcClient.bottom) {
			ScrollTo(lineMove - LinesOnScreen() + 1);
			Redraw();
		} else if (pt.y < rcClient.top) {
			ScrollTo(lineMove);
			Redraw();
		}
		EnsureCaretVisible(false, false, true);

		if (hotspot.Valid() && !PointIsHotspot(pt))
			SetHotSpotRange(NULL);

		if (hotSpotClickPos != INVALID_POSITION && PositionFromLocation(pt,true,true) != hotSpotClickPos) {
			if (inDragDrop == ddNone) {
				DisplayCursor(Window::cursorText);
			}
			hotSpotClickPos = INVALID_POSITION;
		}

	} else {
		if (vs.fixedColumnWidth > 0) {	// There is a margin
			if (PointInSelMargin(pt)) {
				DisplayCursor(GetMarginCursor(pt));
				SetHotSpotRange(NULL);
				return; 	// No need to test for selection
			}
		}
		// Display regular (drag) cursor over selection
		if (PointInSelection(pt) && !SelectionEmpty()) {
			DisplayCursor(Window::cursorArrow);
		} else {
			SetHoverIndicatorPoint(pt);
			if (PointIsHotspot(pt)) {
				DisplayCursor(Window::cursorHand);
				SetHotSpotRange(&pt);
			} else {
				if (hoverIndicatorPos != invalidPosition)
					DisplayCursor(Window::cursorHand);
				else
					DisplayCursor(Window::cursorText);
				SetHotSpotRange(NULL);
			}
		}
	}
}

void Editor::ButtonMove(Point pt) {
	ButtonMoveWithModifiers(pt, 0);
}

void Editor::ButtonUp(Point pt, unsigned int curTime, bool ctrl) {
	//Platform::DebugPrintf("ButtonUp %d %d\n", HaveMouseCapture(), inDragDrop);
	SelectionPosition newPos = SPositionFromLocation(pt, false, false,
		AllowVirtualSpace(virtualSpaceOptions, sel.IsRectangular()));
	if (hoverIndicatorPos != INVALID_POSITION)
		InvalidateRange(newPos.Position(), newPos.Position() + 1);
	newPos = MovePositionOutsideChar(newPos, sel.MainCaret() - newPos.Position());
	if (inDragDrop == ddInitial) {
		inDragDrop = ddNone;
		SetEmptySelection(newPos);
		selectionType = selChar;
		originalAnchorPos = sel.MainCaret();
	}
	if (hotSpotClickPos != INVALID_POSITION && PointIsHotspot(pt)) {
		hotSpotClickPos = INVALID_POSITION;
		SelectionPosition newCharPos = SPositionFromLocation(pt, false, true, false);
		newCharPos = MovePositionOutsideChar(newCharPos, -1);
		NotifyHotSpotReleaseClick(newCharPos.Position(), ctrl ? SCI_CTRL : 0);
	}
	if (HaveMouseCapture()) {
		if (PointInSelMargin(pt)) {
			DisplayCursor(GetMarginCursor(pt));
		} else {
			DisplayCursor(Window::cursorText);
			SetHotSpotRange(NULL);
		}
		ptMouseLast = pt;
		SetMouseCapture(false);
		if (FineTickerAvailable()) {
			FineTickerCancel(tickScroll);
		}
		NotifyIndicatorClick(false, newPos.Position(), 0);
		if (inDragDrop == ddDragging) {
			SelectionPosition selStart = SelectionStart();
			SelectionPosition selEnd = SelectionEnd();
			if (selStart < selEnd) {
				if (drag.Length()) {
					const int length = static_cast<int>(drag.Length());
					if (ctrl) {
						const int lengthInserted = pdoc->InsertString(
							newPos.Position(), drag.Data(), length);
						if (lengthInserted > 0) {
							SetSelection(newPos.Position(), newPos.Position() + lengthInserted);
						}
					} else if (newPos < selStart) {
						pdoc->DeleteChars(selStart.Position(), static_cast<int>(drag.Length()));
						const int lengthInserted = pdoc->InsertString(
							newPos.Position(), drag.Data(), length);
						if (lengthInserted > 0) {
							SetSelection(newPos.Position(), newPos.Position() + lengthInserted);
						}
					} else if (newPos > selEnd) {
						pdoc->DeleteChars(selStart.Position(), static_cast<int>(drag.Length()));
						newPos.Add(-static_cast<int>(drag.Length()));
						const int lengthInserted = pdoc->InsertString(
							newPos.Position(), drag.Data(), length);
						if (lengthInserted > 0) {
							SetSelection(newPos.Position(), newPos.Position() + lengthInserted);
						}
					} else {
						SetEmptySelection(newPos.Position());
					}
					drag.Clear();
				}
				selectionType = selChar;
			}
		} else {
			if (selectionType == selChar) {
				if (sel.Count() > 1) {
					sel.RangeMain() =
						SelectionRange(newPos, sel.Range(sel.Count() - 1).anchor);
					InvalidateSelection(sel.RangeMain(), true);
				} else {
					SetSelection(newPos, sel.RangeMain().anchor);
				}
			}
			sel.CommitTentative();
		}
		SetRectangularRange();
		lastClickTime = curTime;
		lastClick = pt;
		lastXChosen = static_cast<int>(pt.x) + xOffset;
		if (sel.selType == Selection::selStream) {
			SetLastXChosen();
		}
		inDragDrop = ddNone;
		EnsureCaretVisible(false);
	}
}

// Called frequently to perform background UI including
// caret blinking and automatic scrolling.
void Editor::Tick() {
	if (HaveMouseCapture()) {
		// Auto scroll
		ButtonMove(ptMouseLast);
	}
	if (caret.period > 0) {
		timer.ticksToWait -= timer.tickSize;
		if (timer.ticksToWait <= 0) {
			caret.on = !caret.on;
			timer.ticksToWait = caret.period;
			if (caret.active) {
				InvalidateCaret();
			}
		}
	}
	if (horizontalScrollBarVisible && trackLineWidth && (view.lineWidthMaxSeen > scrollWidth)) {
		scrollWidth = view.lineWidthMaxSeen;
		SetScrollBars();
	}
	if ((dwellDelay < SC_TIME_FOREVER) &&
	        (ticksToDwell > 0) &&
	        (!HaveMouseCapture()) &&
	        (ptMouseLast.y >= 0)) {
		ticksToDwell -= timer.tickSize;
		if (ticksToDwell <= 0) {
			dwelling = true;
			NotifyDwelling(ptMouseLast, dwelling);
		}
	}
}

bool Editor::Idle() {

	bool idleDone;

	bool wrappingDone = !Wrapping();

	if (!wrappingDone) {
		// Wrap lines during idle.
		WrapLines(wsIdle);
		// No more wrapping
		if (!wrapPending.NeedsWrap())
			wrappingDone = true;
	}

	// Add more idle things to do here, but make sure idleDone is
	// set correctly before the function returns. returning
	// false will stop calling this idle function until SetIdle() is
	// called again.

	idleDone = wrappingDone; // && thatDone && theOtherThingDone...

	return !idleDone;
}

void Editor::SetTicking(bool) {
	// SetTicking is deprecated. In the past it was pure virtual and was overridden in each
	// derived platform class but fine grained timers should now be implemented.
	// Either way, execution should not arrive here so assert failure.
	assert(false);
}

void Editor::TickFor(TickReason reason) {
	switch (reason) {
		case tickCaret:
			caret.on = !caret.on;
			if (caret.active) {
				InvalidateCaret();
			}
			break;
		case tickScroll:
			// Auto scroll
			ButtonMove(ptMouseLast);
			break;
		case tickWiden:
			SetScrollBars();
			FineTickerCancel(tickWiden);
			break;
		case tickDwell:
			if ((!HaveMouseCapture()) &&
				(ptMouseLast.y >= 0)) {
				dwelling = true;
				NotifyDwelling(ptMouseLast, dwelling);
			}
			FineTickerCancel(tickDwell);
			break;
		default:
			// tickPlatform handled by subclass
			break;
	}
}

bool Editor::FineTickerAvailable() {
	return false;
}

// FineTickerStart is be overridden by subclasses that support fine ticking so
// this method should never be called.
bool Editor::FineTickerRunning(TickReason) {
	assert(false);
	return false;
}

// FineTickerStart is be overridden by subclasses that support fine ticking so
// this method should never be called.
void Editor::FineTickerStart(TickReason, int, int) {
	assert(false);
}

// FineTickerCancel is be overridden by subclasses that support fine ticking so
// this method should never be called.
void Editor::FineTickerCancel(TickReason) {
	assert(false);
}

void Editor::SetFocusState(bool focusState) {
	hasFocus = focusState;
	NotifyFocus(hasFocus);
	if (!hasFocus) {
		CancelModes();
	}
	ShowCaretAtCurrentPosition();
}

int Editor::PositionAfterArea(PRectangle rcArea) const {
	// The start of the document line after the display line after the area
	// This often means that the line after a modification is restyled which helps
	// detect multiline comment additions and heals single line comments
	int lineAfter = TopLineOfMain() + static_cast<int>(rcArea.bottom - 1) / vs.lineHeight + 1;
	if (lineAfter < cs.LinesDisplayed())
		return pdoc->LineStart(cs.DocFromDisplay(lineAfter) + 1);
	else
		return pdoc->Length();
}

// Style to a position within the view. If this causes a change at end of last line then
// affects later lines so style all the viewed text.
void Editor::StyleToPositionInView(Position pos) {
	int endWindow = PositionAfterArea(GetClientDrawingRectangle());
	if (pos > endWindow)
		pos = endWindow;
	int styleAtEnd = pdoc->StyleAt(pos-1);
	pdoc->EnsureStyledTo(pos);
	if ((endWindow > pos) && (styleAtEnd != pdoc->StyleAt(pos-1))) {
		// Style at end of line changed so is multi-line change like starting a comment
		// so require rest of window to be styled.
		DiscardOverdraw();	// Prepared bitmaps may be invalid
		// DiscardOverdraw may have truncated client drawing area so recalculate endWindow
		endWindow = PositionAfterArea(GetClientDrawingRectangle());
		pdoc->EnsureStyledTo(endWindow);
	}
}

void Editor::IdleWork() {
	// Style the line after the modification as this allows modifications that change just the
	// line of the modification to heal instead of propagating to the rest of the window.
	if (workNeeded.items & WorkNeeded::workStyle)
		StyleToPositionInView(pdoc->LineStart(pdoc->LineFromPosition(workNeeded.upTo) + 2));

	NotifyUpdateUI();
	workNeeded.Reset();
}

void Editor::QueueIdleWork(WorkNeeded::workItems items, int upTo) {
	workNeeded.Need(items, upTo);
}

bool Editor::PaintContains(PRectangle rc) {
	if (rc.Empty()) {
		return true;
	} else {
		return rcPaint.Contains(rc);
	}
}

bool Editor::PaintContainsMargin() {
	if (wMargin.GetID()) {
		// With separate margin view, paint of text view
		// never contains margin.
		return false;
	}
	PRectangle rcSelMargin = GetClientRectangle();
	rcSelMargin.right = static_cast<XYPOSITION>(vs.textStart);
	return PaintContains(rcSelMargin);
}

void Editor::CheckForChangeOutsidePaint(Range r) {
	if (paintState == painting && !paintingAllText) {
		//Platform::DebugPrintf("Checking range in paint %d-%d\n", r.start, r.end);
		if (!r.Valid())
			return;

		PRectangle rcRange = RectangleFromRange(r, 0);
		PRectangle rcText = GetTextRectangle();
		if (rcRange.top < rcText.top) {
			rcRange.top = rcText.top;
		}
		if (rcRange.bottom > rcText.bottom) {
			rcRange.bottom = rcText.bottom;
		}

		if (!PaintContains(rcRange)) {
			AbandonPaint();
			paintAbandonedByStyling = true;
		}
	}
}

void Editor::SetBraceHighlight(Position pos0, Position pos1, int matchStyle) {
	if ((pos0 != braces[0]) || (pos1 != braces[1]) || (matchStyle != bracesMatchStyle)) {
		if ((braces[0] != pos0) || (matchStyle != bracesMatchStyle)) {
			CheckForChangeOutsidePaint(Range(braces[0]));
			CheckForChangeOutsidePaint(Range(pos0));
			braces[0] = pos0;
		}
		if ((braces[1] != pos1) || (matchStyle != bracesMatchStyle)) {
			CheckForChangeOutsidePaint(Range(braces[1]));
			CheckForChangeOutsidePaint(Range(pos1));
			braces[1] = pos1;
		}
		bracesMatchStyle = matchStyle;
		if (paintState == notPainting) {
			Redraw();
		}
	}
}

void Editor::SetAnnotationHeights(int start, int end) {
	if (vs.annotationVisible) {
		RefreshStyleData();
		bool changedHeight = false;
		for (int line=start; line<end && line<pdoc->LinesTotal(); line++) {
			int linesWrapped = 1;
			if (Wrapping()) {
				AutoSurface surface(this);
				AutoLineLayout ll(view.llc, view.RetrieveLineLayout(line, *this));
				if (surface && ll) {
					view.LayoutLine(*this, line, surface, vs, ll, wrapWidth);
					linesWrapped = ll->lines;
				}
			}
			if (cs.SetHeight(line, pdoc->AnnotationLines(line) + linesWrapped))
				changedHeight = true;
		}
		if (changedHeight) {
			Redraw();
		}
	}
}

void Editor::SetDocPointer(Document *document) {
	//Platform::DebugPrintf("** %x setdoc to %x\n", pdoc, document);
	pdoc->RemoveWatcher(this, 0);
	pdoc->Release();
	if (document == NULL) {
		pdoc = new Document();
	} else {
		pdoc = document;
	}
	pdoc->AddRef();

	// Ensure all positions within document
	sel.Clear();
	targetStart = 0;
	targetEnd = 0;

	braces[0] = invalidPosition;
	braces[1] = invalidPosition;

	vs.ReleaseAllExtendedStyles();

	SetRepresentations();

	// Reset the contraction state to fully shown.
	cs.Clear();
	cs.InsertLines(0, pdoc->LinesTotal() - 1);
	SetAnnotationHeights(0, pdoc->LinesTotal());
	view.llc.Deallocate();
	NeedWrapping();

	hotspot = Range(invalidPosition);
	hoverIndicatorPos = invalidPosition;

	view.ClearAllTabstops();

	pdoc->AddWatcher(this, 0);
	SetScrollBars();
	Redraw();
}

void Editor::SetAnnotationVisible(int visible) {
	if (vs.annotationVisible != visible) {
		bool changedFromOrToHidden = ((vs.annotationVisible != 0) != (visible != 0));
		vs.annotationVisible = visible;
		if (changedFromOrToHidden) {
			int dir = vs.annotationVisible ? 1 : -1;
			for (int line=0; line<pdoc->LinesTotal(); line++) {
				int annotationLines = pdoc->AnnotationLines(line);
				if (annotationLines > 0) {
					cs.SetHeight(line, cs.GetHeight(line) + annotationLines * dir);
				}
			}
		}
		Redraw();
	}
}

/**
 * Recursively expand a fold, making lines visible except where they have an unexpanded parent.
 */
int Editor::ExpandLine(int line) {
	int lineMaxSubord = pdoc->GetLastChild(line);
	line++;
	while (line <= lineMaxSubord) {
		cs.SetVisible(line, line, true);
		int level = pdoc->GetLevel(line);
		if (level & SC_FOLDLEVELHEADERFLAG) {
			if (cs.GetExpanded(line)) {
				line = ExpandLine(line);
			} else {
				line = pdoc->GetLastChild(line);
			}
		}
		line++;
	}
	return lineMaxSubord;
}

void Editor::SetFoldExpanded(int lineDoc, bool expanded) {
	if (cs.SetExpanded(lineDoc, expanded)) {
		RedrawSelMargin();
	}
}

void Editor::FoldLine(int line, int action) {
	if (line >= 0) {
		if (action == SC_FOLDACTION_TOGGLE) {
			if ((pdoc->GetLevel(line) & SC_FOLDLEVELHEADERFLAG) == 0) {
				line = pdoc->GetFoldParent(line);
				if (line < 0)
					return;
			}
			action = (cs.GetExpanded(line)) ? SC_FOLDACTION_CONTRACT : SC_FOLDACTION_EXPAND;
		}

		if (action == SC_FOLDACTION_CONTRACT) {
			int lineMaxSubord = pdoc->GetLastChild(line);
			if (lineMaxSubord > line) {
				cs.SetExpanded(line, 0);
				cs.SetVisible(line + 1, lineMaxSubord, false);

				int lineCurrent = pdoc->LineFromPosition(sel.MainCaret());
				if (lineCurrent > line && lineCurrent <= lineMaxSubord) {
					// This does not re-expand the fold
					EnsureCaretVisible();
				}
			}

		} else {
			if (!(cs.GetVisible(line))) {
				EnsureLineVisible(line, false);
				GoToLine(line);
			}
			cs.SetExpanded(line, 1);
			ExpandLine(line);
		}

		SetScrollBars();
		Redraw();
	}
}

void Editor::FoldExpand(int line, int action, int level) {
	bool expanding = action == SC_FOLDACTION_EXPAND;
	if (action == SC_FOLDACTION_TOGGLE) {
		expanding = !cs.GetExpanded(line);
	}
	SetFoldExpanded(line, expanding);
	if (expanding && (cs.HiddenLines() == 0))
		// Nothing to do
		return;
	int lineMaxSubord = pdoc->GetLastChild(line, level & SC_FOLDLEVELNUMBERMASK);
	line++;
	cs.SetVisible(line, lineMaxSubord, expanding);
	while (line <= lineMaxSubord) {
		int levelLine = pdoc->GetLevel(line);
		if (levelLine & SC_FOLDLEVELHEADERFLAG) {
			SetFoldExpanded(line, expanding);
		}
		line++;
	}
	SetScrollBars();
	Redraw();
}

int Editor::ContractedFoldNext(int lineStart) const {
	for (int line = lineStart; line<pdoc->LinesTotal();) {
		if (!cs.GetExpanded(line) && (pdoc->GetLevel(line) & SC_FOLDLEVELHEADERFLAG))
			return line;
		line = cs.ContractedNext(line+1);
		if (line < 0)
			return -1;
	}

	return -1;
}

/**
 * Recurse up from this line to find any folds that prevent this line from being visible
 * and unfold them all.
 */
void Editor::EnsureLineVisible(int lineDoc, bool enforcePolicy) {

	// In case in need of wrapping to ensure DisplayFromDoc works.
	if (lineDoc >= wrapPending.start)
		WrapLines(wsAll);

	if (!cs.GetVisible(lineDoc)) {
		// Back up to find a non-blank line
		int lookLine = lineDoc;
		int lookLineLevel = pdoc->GetLevel(lookLine);
		while ((lookLine > 0) && (lookLineLevel & SC_FOLDLEVELWHITEFLAG)) {
			lookLineLevel = pdoc->GetLevel(--lookLine);
		}
		int lineParent = pdoc->GetFoldParent(lookLine);
		if (lineParent < 0) {
			// Backed up to a top level line, so try to find parent of initial line
			lineParent = pdoc->GetFoldParent(lineDoc);
		}
		if (lineParent >= 0) {
			if (lineDoc != lineParent)
				EnsureLineVisible(lineParent, enforcePolicy);
			if (!cs.GetExpanded(lineParent)) {
				cs.SetExpanded(lineParent, 1);
				ExpandLine(lineParent);
			}
		}
		SetScrollBars();
		Redraw();
	}
	if (enforcePolicy) {
		int lineDisplay = cs.DisplayFromDoc(lineDoc);
		if (visiblePolicy & VISIBLE_SLOP) {
			if ((topLine > lineDisplay) || ((visiblePolicy & VISIBLE_STRICT) && (topLine + visibleSlop > lineDisplay))) {
				SetTopLine(Platform::Clamp(lineDisplay - visibleSlop, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			} else if ((lineDisplay > topLine + LinesOnScreen() - 1) ||
			        ((visiblePolicy & VISIBLE_STRICT) && (lineDisplay > topLine + LinesOnScreen() - 1 - visibleSlop))) {
				SetTopLine(Platform::Clamp(lineDisplay - LinesOnScreen() + 1 + visibleSlop, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			}
		} else {
			if ((topLine > lineDisplay) || (lineDisplay > topLine + LinesOnScreen() - 1) || (visiblePolicy & VISIBLE_STRICT)) {
				SetTopLine(Platform::Clamp(lineDisplay - LinesOnScreen() / 2 + 1, 0, MaxScrollPos()));
				SetVerticalScrollPos();
				Redraw();
			}
		}
	}
}

void Editor::FoldAll(int action) {
	pdoc->EnsureStyledTo(pdoc->Length());
	int maxLine = pdoc->LinesTotal();
	bool expanding = action == SC_FOLDACTION_EXPAND;
	if (action == SC_FOLDACTION_TOGGLE) {
		// Discover current state
		for (int lineSeek = 0; lineSeek < maxLine; lineSeek++) {
			if (pdoc->GetLevel(lineSeek) & SC_FOLDLEVELHEADERFLAG) {
				expanding = !cs.GetExpanded(lineSeek);
				break;
			}
		}
	}
	if (expanding) {
		cs.SetVisible(0, maxLine-1, true);
		for (int line = 0; line < maxLine; line++) {
			int levelLine = pdoc->GetLevel(line);
			if (levelLine & SC_FOLDLEVELHEADERFLAG) {
				SetFoldExpanded(line, true);
			}
		}
	} else {
		for (int line = 0; line < maxLine; line++) {
			int level = pdoc->GetLevel(line);
			if ((level & SC_FOLDLEVELHEADERFLAG) &&
					(SC_FOLDLEVELBASE == (level & SC_FOLDLEVELNUMBERMASK))) {
				SetFoldExpanded(line, false);
				int lineMaxSubord = pdoc->GetLastChild(line, -1);
				if (lineMaxSubord > line) {
					cs.SetVisible(line + 1, lineMaxSubord, false);
				}
			}
		}
	}
	SetScrollBars();
	Redraw();
}

void Editor::FoldChanged(int line, int levelNow, int levelPrev) {
	if (levelNow & SC_FOLDLEVELHEADERFLAG) {
		if (!(levelPrev & SC_FOLDLEVELHEADERFLAG)) {
			// Adding a fold point.
			if (cs.SetExpanded(line, true)) {
				RedrawSelMargin();
			}
			FoldExpand(line, SC_FOLDACTION_EXPAND, levelPrev);
		}
	} else if (levelPrev & SC_FOLDLEVELHEADERFLAG) {
		if (!cs.GetExpanded(line)) {
			// Removing the fold from one that has been contracted so should expand
			// otherwise lines are left invisible with no way to make them visible
			if (cs.SetExpanded(line, true)) {
				RedrawSelMargin();
			}
			FoldExpand(line, SC_FOLDACTION_EXPAND, levelPrev);
		}
	}
	if (!(levelNow & SC_FOLDLEVELWHITEFLAG) &&
	        ((levelPrev & SC_FOLDLEVELNUMBERMASK) > (levelNow & SC_FOLDLEVELNUMBERMASK))) {
		if (cs.HiddenLines()) {
			// See if should still be hidden
			int parentLine = pdoc->GetFoldParent(line);
			if ((parentLine < 0) || (cs.GetExpanded(parentLine) && cs.GetVisible(parentLine))) {
				cs.SetVisible(line, line, true);
				SetScrollBars();
				Redraw();
			}
		}
	}
}

void Editor::NeedShown(int pos, int len) {
	if (foldAutomatic & SC_AUTOMATICFOLD_SHOW) {
		int lineStart = pdoc->LineFromPosition(pos);
		int lineEnd = pdoc->LineFromPosition(pos+len);
		for (int line = lineStart; line <= lineEnd; line++) {
			EnsureLineVisible(line, false);
		}
	} else {
		NotifyNeedShown(pos, len);
	}
}

int Editor::GetTag(char *tagValue, int tagNumber) {
	const char *text = 0;
	int length = 0;
	if ((tagNumber >= 1) && (tagNumber <= 9)) {
		char name[3] = "\\?";
		name[1] = static_cast<char>(tagNumber + '0');
		length = 2;
		text = pdoc->SubstituteByPosition(name, &length);
	}
	if (tagValue) {
		if (text)
			memcpy(tagValue, text, length + 1);
		else
			*tagValue = '\0';
	}
	return length;
}

int Editor::ReplaceTarget(bool replacePatterns, const char *text, int length) {
	UndoGroup ug(pdoc);
	if (length == -1)
		length = istrlen(text);
	if (replacePatterns) {
		text = pdoc->SubstituteByPosition(text, &length);
		if (!text) {
			return 0;
		}
	}
	if (targetStart != targetEnd)
		pdoc->DeleteChars(targetStart, targetEnd - targetStart);
	targetEnd = targetStart;
	const int lengthInserted = pdoc->InsertString(targetStart, text, length);
	targetEnd = targetStart + lengthInserted;
	return length;
}

bool Editor::IsUnicodeMode() const {
	return pdoc && (SC_CP_UTF8 == pdoc->dbcsCodePage);
}

int Editor::CodePage() const {
	if (pdoc)
		return pdoc->dbcsCodePage;
	else
		return 0;
}

int Editor::WrapCount(int line) {
	AutoSurface surface(this);
	AutoLineLayout ll(view.llc, view.RetrieveLineLayout(line, *this));

	if (surface && ll) {
		view.LayoutLine(*this, line, surface, vs, ll, wrapWidth);
		return ll->lines;
	} else {
		return 1;
	}
}

void Editor::AddStyledText(char *buffer, int appendLength) {
	// The buffer consists of alternating character bytes and style bytes
	int textLength = appendLength / 2;
	std::string text(textLength, '\0');
	int i;
	for (i = 0; i < textLength; i++) {
		text[i] = buffer[i*2];
	}
	const int lengthInserted = pdoc->InsertString(CurrentPosition(), text.c_str(), textLength);
	for (i = 0; i < textLength; i++) {
		text[i] = buffer[i*2+1];
	}
	pdoc->StartStyling(CurrentPosition(), static_cast<unsigned char>(0xff));
	pdoc->SetStyles(textLength, text.c_str());
	SetEmptySelection(sel.MainCaret() + lengthInserted);
}

static bool ValidMargin(uptr_t wParam) {
	return wParam <= SC_MAX_MARGIN;
}

static char *CharPtrFromSPtr(sptr_t lParam) {
	return reinterpret_cast<char *>(lParam);
}

void Editor::StyleSetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	vs.EnsureStyle(wParam);
	switch (iMessage) {
	case SCI_STYLESETFORE:
		vs.styles[wParam].fore = ColourDesired(static_cast<long>(lParam));
		break;
	case SCI_STYLESETBACK:
		vs.styles[wParam].back = ColourDesired(static_cast<long>(lParam));
		break;
	case SCI_STYLESETBOLD:
		vs.styles[wParam].weight = lParam != 0 ? SC_WEIGHT_BOLD : SC_WEIGHT_NORMAL;
		break;
	case SCI_STYLESETWEIGHT:
		vs.styles[wParam].weight = static_cast<int>(lParam);
		break;
	case SCI_STYLESETITALIC:
		vs.styles[wParam].italic = lParam != 0;
		break;
	case SCI_STYLESETEOLFILLED:
		vs.styles[wParam].eolFilled = lParam != 0;
		break;
	case SCI_STYLESETSIZE:
		vs.styles[wParam].size = static_cast<int>(lParam * SC_FONT_SIZE_MULTIPLIER);
		break;
	case SCI_STYLESETSIZEFRACTIONAL:
		vs.styles[wParam].size = static_cast<int>(lParam);
		break;
	case SCI_STYLESETFONT:
		if (lParam != 0) {
			vs.SetStyleFontName(static_cast<int>(wParam), CharPtrFromSPtr(lParam));
		}
		break;
	case SCI_STYLESETUNDERLINE:
		vs.styles[wParam].underline = lParam != 0;
		break;
	case SCI_STYLESETCASE:
		vs.styles[wParam].caseForce = static_cast<Style::ecaseForced>(lParam);
		break;
	case SCI_STYLESETCHARACTERSET:
		vs.styles[wParam].characterSet = static_cast<int>(lParam);
		pdoc->SetCaseFolder(NULL);
		break;
	case SCI_STYLESETVISIBLE:
		vs.styles[wParam].visible = lParam != 0;
		break;
	case SCI_STYLESETCHANGEABLE:
		vs.styles[wParam].changeable = lParam != 0;
		break;
	case SCI_STYLESETHOTSPOT:
		vs.styles[wParam].hotspot = lParam != 0;
		break;
	}
	InvalidateStyleRedraw();
}

sptr_t Editor::StyleGetMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	vs.EnsureStyle(wParam);
	switch (iMessage) {
	case SCI_STYLEGETFORE:
		return vs.styles[wParam].fore.AsLong();
	case SCI_STYLEGETBACK:
		return vs.styles[wParam].back.AsLong();
	case SCI_STYLEGETBOLD:
		return vs.styles[wParam].weight > SC_WEIGHT_NORMAL;
	case SCI_STYLEGETWEIGHT:
		return vs.styles[wParam].weight;
	case SCI_STYLEGETITALIC:
		return vs.styles[wParam].italic ? 1 : 0;
	case SCI_STYLEGETEOLFILLED:
		return vs.styles[wParam].eolFilled ? 1 : 0;
	case SCI_STYLEGETSIZE:
		return vs.styles[wParam].size / SC_FONT_SIZE_MULTIPLIER;
	case SCI_STYLEGETSIZEFRACTIONAL:
		return vs.styles[wParam].size;
	case SCI_STYLEGETFONT:
		return StringResult(lParam, vs.styles[wParam].fontName);
	case SCI_STYLEGETUNDERLINE:
		return vs.styles[wParam].underline ? 1 : 0;
	case SCI_STYLEGETCASE:
		return static_cast<int>(vs.styles[wParam].caseForce);
	case SCI_STYLEGETCHARACTERSET:
		return vs.styles[wParam].characterSet;
	case SCI_STYLEGETVISIBLE:
		return vs.styles[wParam].visible ? 1 : 0;
	case SCI_STYLEGETCHANGEABLE:
		return vs.styles[wParam].changeable ? 1 : 0;
	case SCI_STYLEGETHOTSPOT:
		return vs.styles[wParam].hotspot ? 1 : 0;
	}
	return 0;
}

sptr_t Editor::StringResult(sptr_t lParam, const char *val) {
	const size_t len = val ? strlen(val) : 0;
	if (lParam) {
		char *ptr = CharPtrFromSPtr(lParam);
		if (val)
			memcpy(ptr, val, len+1);
		else
			*ptr = 0;
	}
	return len;	// Not including NUL
}

sptr_t Editor::BytesResult(sptr_t lParam, const unsigned char *val, size_t len) {
	// No NUL termination: len is number of valid/displayed bytes
	if ((lParam) && (len > 0)) {
		char *ptr = CharPtrFromSPtr(lParam);
		if (val)
			memcpy(ptr, val, len);
		else
			*ptr = 0;
	}
	return val ? len : 0;
}

sptr_t Editor::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	//Platform::DebugPrintf("S start wnd proc %d %d %d\n",iMessage, wParam, lParam);

	// Optional macro recording hook
	if (recordingMacro)
		NotifyMacroRecord(iMessage, wParam, lParam);

	switch (iMessage) {

	case SCI_GETTEXT: {
			if (lParam == 0)
				return pdoc->Length() + 1;
			if (wParam == 0)
				return 0;
			char *ptr = CharPtrFromSPtr(lParam);
			unsigned int iChar = 0;
			for (; iChar < wParam - 1; iChar++)
				ptr[iChar] = pdoc->CharAt(iChar);
			ptr[iChar] = '\0';
			return iChar;
		}

	case SCI_SETTEXT: {
			if (lParam == 0)
				return 0;
			UndoGroup ug(pdoc);
			pdoc->DeleteChars(0, pdoc->Length());
			SetEmptySelection(0);
			const char *text = CharPtrFromSPtr(lParam);
			pdoc->InsertString(0, text, istrlen(text));
			return 1;
		}

	case SCI_GETTEXTLENGTH:
		return pdoc->Length();

	case SCI_CUT:
		Cut();
		SetLastXChosen();
		break;

	case SCI_COPY:
		Copy();
		break;

	case SCI_COPYALLOWLINE:
		CopyAllowLine();
		break;

	case SCI_VERTICALCENTRECARET:
		VerticalCentreCaret();
		break;

	case SCI_MOVESELECTEDLINESUP:
		MoveSelectedLinesUp();
		break;

	case SCI_MOVESELECTEDLINESDOWN:
		MoveSelectedLinesDown();
		break;

	case SCI_COPYRANGE:
		CopyRangeToClipboard(static_cast<int>(wParam), static_cast<int>(lParam));
		break;

	case SCI_COPYTEXT:
		CopyText(static_cast<int>(wParam), CharPtrFromSPtr(lParam));
		break;

	case SCI_PASTE:
		Paste();
		if ((caretSticky == SC_CARETSTICKY_OFF) || (caretSticky == SC_CARETSTICKY_WHITESPACE)) {
			SetLastXChosen();
		}
		EnsureCaretVisible();
		break;

	case SCI_CLEAR:
		Clear();
		SetLastXChosen();
		EnsureCaretVisible();
		break;

	case SCI_UNDO:
		Undo();
		SetLastXChosen();
		break;

	case SCI_CANUNDO:
		return (pdoc->CanUndo() && !pdoc->IsReadOnly()) ? 1 : 0;

	case SCI_EMPTYUNDOBUFFER:
		pdoc->DeleteUndoHistory();
		return 0;

	case SCI_GETFIRSTVISIBLELINE:
		return topLine;

	case SCI_SETFIRSTVISIBLELINE:
		ScrollTo(static_cast<int>(wParam));
		break;

	case SCI_GETLINE: {	// Risk of overwriting the end of the buffer
			int lineStart = pdoc->LineStart(static_cast<int>(wParam));
			int lineEnd = pdoc->LineStart(static_cast<int>(wParam + 1));
			if (lParam == 0) {
				return lineEnd - lineStart;
			}
			char *ptr = CharPtrFromSPtr(lParam);
			int iPlace = 0;
			for (int iChar = lineStart; iChar < lineEnd; iChar++) {
				ptr[iPlace++] = pdoc->CharAt(iChar);
			}
			return iPlace;
		}

	case SCI_GETLINECOUNT:
		if (pdoc->LinesTotal() == 0)
			return 1;
		else
			return pdoc->LinesTotal();

	case SCI_GETMODIFY:
		return !pdoc->IsSavePoint();

	case SCI_SETSEL: {
			int nStart = static_cast<int>(wParam);
			int nEnd = static_cast<int>(lParam);
			if (nEnd < 0)
				nEnd = pdoc->Length();
			if (nStart < 0)
				nStart = nEnd; 	// Remove selection
			InvalidateSelection(SelectionRange(nStart, nEnd));
			sel.Clear();
			sel.selType = Selection::selStream;
			SetSelection(nEnd, nStart);
			EnsureCaretVisible();
		}
		break;

	case SCI_GETSELTEXT: {
			SelectionText selectedText;
			CopySelectionRange(&selectedText);
			if (lParam == 0) {
				return selectedText.LengthWithTerminator();
			} else {
				char *ptr = CharPtrFromSPtr(lParam);
				unsigned int iChar = 0;
				if (selectedText.Length()) {
					for (; iChar < selectedText.LengthWithTerminator(); iChar++)
						ptr[iChar] = selectedText.Data()[iChar];
				} else {
					ptr[0] = '\0';
				}
				return iChar;
			}
		}

	case SCI_LINEFROMPOSITION:
		if (static_cast<int>(wParam) < 0)
			return 0;
		return pdoc->LineFromPosition(static_cast<int>(wParam));

	case SCI_POSITIONFROMLINE:
		if (static_cast<int>(wParam) < 0)
			wParam = pdoc->LineFromPosition(SelectionStart().Position());
		if (wParam == 0)
			return 0; 	// Even if there is no text, there is a first line that starts at 0
		if (static_cast<int>(wParam) > pdoc->LinesTotal())
			return -1;
		//if (wParam > pdoc->LineFromPosition(pdoc->Length()))	// Useful test, anyway...
		//	return -1;
		return pdoc->LineStart(static_cast<int>(wParam));

		// Replacement of the old Scintilla interpretation of EM_LINELENGTH
	case SCI_LINELENGTH:
		if ((static_cast<int>(wParam) < 0) ||
		        (static_cast<int>(wParam) > pdoc->LineFromPosition(pdoc->Length())))
			return 0;
		return pdoc->LineStart(static_cast<int>(wParam) + 1) - pdoc->LineStart(static_cast<int>(wParam));

	case SCI_REPLACESEL: {
			if (lParam == 0)
				return 0;
			UndoGroup ug(pdoc);
			ClearSelection();
			char *replacement = CharPtrFromSPtr(lParam);
			const int lengthInserted = pdoc->InsertString(
				sel.MainCaret(), replacement, istrlen(replacement));
			SetEmptySelection(sel.MainCaret() + lengthInserted);
			EnsureCaretVisible();
		}
		break;

	case SCI_SETTARGETSTART:
		targetStart = static_cast<int>(wParam);
		break;

	case SCI_GETTARGETSTART:
		return targetStart;

	case SCI_SETTARGETEND:
		targetEnd = static_cast<int>(wParam);
		break;

	case SCI_GETTARGETEND:
		return targetEnd;

	case SCI_SETTARGETRANGE:
		targetStart = static_cast<int>(wParam);
		targetEnd = static_cast<int>(lParam);
		break;

	case SCI_TARGETFROMSELECTION:
		if (sel.MainCaret() < sel.MainAnchor()) {
			targetStart = sel.MainCaret();
			targetEnd = sel.MainAnchor();
		} else {
			targetStart = sel.MainAnchor();
			targetEnd = sel.MainCaret();
		}
		break;

	case SCI_GETTARGETTEXT: {
			std::string text = RangeText(targetStart, targetEnd);
			return BytesResult(lParam, reinterpret_cast<const unsigned char *>(text.c_str()), text.length());
		}

	case SCI_REPLACETARGET:
		PLATFORM_ASSERT(lParam);
		return ReplaceTarget(false, CharPtrFromSPtr(lParam), static_cast<int>(wParam));

	case SCI_REPLACETARGETRE:
		PLATFORM_ASSERT(lParam);
		return ReplaceTarget(true, CharPtrFromSPtr(lParam), static_cast<int>(wParam));

	case SCI_SEARCHINTARGET:
		PLATFORM_ASSERT(lParam);
		return SearchInTarget(CharPtrFromSPtr(lParam), static_cast<int>(wParam));

	case SCI_SETSEARCHFLAGS:
		searchFlags = static_cast<int>(wParam);
		break;

	case SCI_GETSEARCHFLAGS:
		return searchFlags;

	case SCI_GETTAG:
		return GetTag(CharPtrFromSPtr(lParam), static_cast<int>(wParam));

	case SCI_POSITIONBEFORE:
		return pdoc->MovePositionOutsideChar(static_cast<int>(wParam) - 1, -1, true);

	case SCI_POSITIONAFTER:
		return pdoc->MovePositionOutsideChar(static_cast<int>(wParam) + 1, 1, true);

	case SCI_POSITIONRELATIVE:
		return Platform::Clamp(pdoc->GetRelativePosition(static_cast<int>(wParam), static_cast<int>(lParam)), 0, pdoc->Length());

	case SCI_LINESCROLL:
		ScrollTo(topLine + static_cast<int>(lParam));
		HorizontalScrollTo(xOffset + static_cast<int>(wParam)* static_cast<int>(vs.spaceWidth));
		return 1;

	case SCI_SETXOFFSET:
		xOffset = static_cast<int>(wParam);
		ContainerNeedsUpdate(SC_UPDATE_H_SCROLL);
		SetHorizontalScrollPos();
		Redraw();
		break;

	case SCI_GETXOFFSET:
		return xOffset;

	case SCI_CHOOSECARETX:
		SetLastXChosen();
		break;

	case SCI_SCROLLCARET:
		EnsureCaretVisible();
		break;

	case SCI_SETREADONLY:
		pdoc->SetReadOnly(wParam != 0);
		return 1;

	case SCI_GETREADONLY:
		return pdoc->IsReadOnly();

	case SCI_CANPASTE:
		return CanPaste();

	case SCI_POINTXFROMPOSITION:
		if (lParam < 0) {
			return 0;
		} else {
			Point pt = LocationFromPosition(static_cast<int>(lParam));
			// Convert to view-relative
			return static_cast<int>(pt.x) - vs.textStart + vs.fixedColumnWidth;
		}

	case SCI_POINTYFROMPOSITION:
		if (lParam < 0) {
			return 0;
		} else {
			Point pt = LocationFromPosition(static_cast<int>(lParam));
			return static_cast<int>(pt.y);
		}

	case SCI_FINDTEXT:
		return FindText(wParam, lParam);

	case SCI_GETTEXTRANGE: {
			if (lParam == 0)
				return 0;
			Sci_TextRange *tr = reinterpret_cast<Sci_TextRange *>(lParam);
			int cpMax = static_cast<int>(tr->chrg.cpMax);
			if (cpMax == -1)
				cpMax = pdoc->Length();
			PLATFORM_ASSERT(cpMax <= pdoc->Length());
			int len = static_cast<int>(cpMax - tr->chrg.cpMin); 	// No -1 as cpMin and cpMax are referring to inter character positions
			pdoc->GetCharRange(tr->lpstrText, static_cast<int>(tr->chrg.cpMin), len);
			// Spec says copied text is terminated with a NUL
			tr->lpstrText[len] = '\0';
			return len; 	// Not including NUL
		}

	case SCI_HIDESELECTION:
		view.hideSelection = wParam != 0;
		Redraw();
		break;

	case SCI_FORMATRANGE:
		return FormatRange(wParam != 0, reinterpret_cast<Sci_RangeToFormat *>(lParam));

	case SCI_GETMARGINLEFT:
		return vs.leftMarginWidth;

	case SCI_GETMARGINRIGHT:
		return vs.rightMarginWidth;

	case SCI_SETMARGINLEFT:
		lastXChosen += static_cast<int>(lParam) - vs.leftMarginWidth;
		vs.leftMarginWidth = static_cast<int>(lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETMARGINRIGHT:
		vs.rightMarginWidth = static_cast<int>(lParam);
		InvalidateStyleRedraw();
		break;

		// Control specific mesages

	case SCI_ADDTEXT: {
			if (lParam == 0)
				return 0;
			const int lengthInserted = pdoc->InsertString(
				CurrentPosition(), CharPtrFromSPtr(lParam), static_cast<int>(wParam));
			SetEmptySelection(sel.MainCaret() + lengthInserted);
			return 0;
		}

	case SCI_ADDSTYLEDTEXT:
		if (lParam)
			AddStyledText(CharPtrFromSPtr(lParam), static_cast<int>(wParam));
		return 0;

	case SCI_INSERTTEXT: {
			if (lParam == 0)
				return 0;
			int insertPos = static_cast<int>(wParam);
			if (static_cast<int>(wParam) == -1)
				insertPos = CurrentPosition();
			int newCurrent = CurrentPosition();
			char *sz = CharPtrFromSPtr(lParam);
			const int lengthInserted = pdoc->InsertString(insertPos, sz, istrlen(sz));
			if (newCurrent > insertPos)
				newCurrent += lengthInserted;
			SetEmptySelection(newCurrent);
			return 0;
		}

	case SCI_CHANGEINSERTION:
		PLATFORM_ASSERT(lParam);
		pdoc->ChangeInsertion(CharPtrFromSPtr(lParam), static_cast<int>(wParam));
		return 0;

	case SCI_APPENDTEXT:
		pdoc->InsertString(pdoc->Length(), CharPtrFromSPtr(lParam), static_cast<int>(wParam));
		return 0;

	case SCI_CLEARALL:
		ClearAll();
		return 0;

	case SCI_DELETERANGE:
		pdoc->DeleteChars(static_cast<int>(wParam), static_cast<int>(lParam));
		return 0;

	case SCI_CLEARDOCUMENTSTYLE:
		ClearDocumentStyle();
		return 0;

	case SCI_SETUNDOCOLLECTION:
		pdoc->SetUndoCollection(wParam != 0);
		return 0;

	case SCI_GETUNDOCOLLECTION:
		return pdoc->IsCollectingUndo();

	case SCI_BEGINUNDOACTION:
		pdoc->BeginUndoAction();
		return 0;

	case SCI_ENDUNDOACTION:
		pdoc->EndUndoAction();
		return 0;

	case SCI_GETCARETPERIOD:
		return caret.period;

	case SCI_SETCARETPERIOD:
		CaretSetPeriod(static_cast<int>(wParam));
		break;

	case SCI_GETWORDCHARS:
		return pdoc->GetCharsOfClass(CharClassify::ccWord, reinterpret_cast<unsigned char *>(lParam));

	case SCI_SETWORDCHARS: {
			pdoc->SetDefaultCharClasses(false);
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(reinterpret_cast<unsigned char *>(lParam), CharClassify::ccWord);
		}
		break;

	case SCI_GETWHITESPACECHARS:
		return pdoc->GetCharsOfClass(CharClassify::ccSpace, reinterpret_cast<unsigned char *>(lParam));

	case SCI_SETWHITESPACECHARS: {
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(reinterpret_cast<unsigned char *>(lParam), CharClassify::ccSpace);
		}
		break;

	case SCI_GETPUNCTUATIONCHARS:
		return pdoc->GetCharsOfClass(CharClassify::ccPunctuation, reinterpret_cast<unsigned char *>(lParam));

	case SCI_SETPUNCTUATIONCHARS: {
			if (lParam == 0)
				return 0;
			pdoc->SetCharClasses(reinterpret_cast<unsigned char *>(lParam), CharClassify::ccPunctuation);
		}
		break;

	case SCI_SETCHARSDEFAULT:
		pdoc->SetDefaultCharClasses(true);
		break;

	case SCI_GETLENGTH:
		return pdoc->Length();

	case SCI_ALLOCATE:
		pdoc->Allocate(static_cast<int>(wParam));
		break;

	case SCI_GETCHARAT:
		return pdoc->CharAt(static_cast<int>(wParam));

	case SCI_SETCURRENTPOS:
		if (sel.IsRectangular()) {
			sel.Rectangular().caret.SetPosition(static_cast<int>(wParam));
			SetRectangularRange();
			Redraw();
		} else {
			SetSelection(static_cast<int>(wParam), sel.MainAnchor());
		}
		break;

	case SCI_GETCURRENTPOS:
		return sel.IsRectangular() ? sel.Rectangular().caret.Position() : sel.MainCaret();

	case SCI_SETANCHOR:
		if (sel.IsRectangular()) {
			sel.Rectangular().anchor.SetPosition(static_cast<int>(wParam));
			SetRectangularRange();
			Redraw();
		} else {
			SetSelection(sel.MainCaret(), static_cast<int>(wParam));
		}
		break;

	case SCI_GETANCHOR:
		return sel.IsRectangular() ? sel.Rectangular().anchor.Position() : sel.MainAnchor();

	case SCI_SETSELECTIONSTART:
		SetSelection(Platform::Maximum(sel.MainCaret(), static_cast<int>(wParam)), static_cast<int>(wParam));
		break;

	case SCI_GETSELECTIONSTART:
		return sel.LimitsForRectangularElseMain().start.Position();

	case SCI_SETSELECTIONEND:
		SetSelection(static_cast<int>(wParam), Platform::Minimum(sel.MainAnchor(), static_cast<int>(wParam)));
		break;

	case SCI_GETSELECTIONEND:
		return sel.LimitsForRectangularElseMain().end.Position();

	case SCI_SETEMPTYSELECTION:
		SetEmptySelection(static_cast<int>(wParam));
		break;

	case SCI_SETPRINTMAGNIFICATION:
		view.printParameters.magnification = static_cast<int>(wParam);
		break;

	case SCI_GETPRINTMAGNIFICATION:
		return view.printParameters.magnification;

	case SCI_SETPRINTCOLOURMODE:
		view.printParameters.colourMode = static_cast<int>(wParam);
		break;

	case SCI_GETPRINTCOLOURMODE:
		return view.printParameters.colourMode;

	case SCI_SETPRINTWRAPMODE:
		view.printParameters.wrapState = (wParam == SC_WRAP_WORD) ? eWrapWord : eWrapNone;
		break;

	case SCI_GETPRINTWRAPMODE:
		return view.printParameters.wrapState;

	case SCI_GETSTYLEAT:
		if (static_cast<int>(wParam) >= pdoc->Length())
			return 0;
		else
			return pdoc->StyleAt(static_cast<int>(wParam));

	case SCI_REDO:
		Redo();
		break;

	case SCI_SELECTALL:
		SelectAll();
		break;

	case SCI_SETSAVEPOINT:
		pdoc->SetSavePoint();
		break;

	case SCI_GETSTYLEDTEXT: {
			if (lParam == 0)
				return 0;
			Sci_TextRange *tr = reinterpret_cast<Sci_TextRange *>(lParam);
			int iPlace = 0;
			for (long iChar = tr->chrg.cpMin; iChar < tr->chrg.cpMax; iChar++) {
				tr->lpstrText[iPlace++] = pdoc->CharAt(static_cast<int>(iChar));
				tr->lpstrText[iPlace++] = pdoc->StyleAt(static_cast<int>(iChar));
			}
			tr->lpstrText[iPlace] = '\0';
			tr->lpstrText[iPlace + 1] = '\0';
			return iPlace;
		}

	case SCI_CANREDO:
		return (pdoc->CanRedo() && !pdoc->IsReadOnly()) ? 1 : 0;

	case SCI_MARKERLINEFROMHANDLE:
		return pdoc->LineFromHandle(static_cast<int>(wParam));

	case SCI_MARKERDELETEHANDLE:
		pdoc->DeleteMarkFromHandle(static_cast<int>(wParam));
		break;

	case SCI_GETVIEWWS:
		return vs.viewWhitespace;

	case SCI_SETVIEWWS:
		vs.viewWhitespace = static_cast<WhiteSpaceVisibility>(wParam);
		Redraw();
		break;

	case SCI_GETWHITESPACESIZE:
		return vs.whitespaceSize;

	case SCI_SETWHITESPACESIZE:
		vs.whitespaceSize = static_cast<int>(wParam);
		Redraw();
		break;

	case SCI_POSITIONFROMPOINT:
		return PositionFromLocation(Point::FromInts(static_cast<int>(wParam) - vs.ExternalMarginWidth(), static_cast<int>(lParam)),
					    false, false);

	case SCI_POSITIONFROMPOINTCLOSE:
		return PositionFromLocation(Point::FromInts(static_cast<int>(wParam) - vs.ExternalMarginWidth(), static_cast<int>(lParam)),
					    true, false);

	case SCI_CHARPOSITIONFROMPOINT:
		return PositionFromLocation(Point::FromInts(static_cast<int>(wParam) - vs.ExternalMarginWidth(), static_cast<int>(lParam)),
					    false, true);

	case SCI_CHARPOSITIONFROMPOINTCLOSE:
		return PositionFromLocation(Point::FromInts(static_cast<int>(wParam) - vs.ExternalMarginWidth(), static_cast<int>(lParam)),
					    true, true);

	case SCI_GOTOLINE:
		GoToLine(static_cast<int>(wParam));
		break;

	case SCI_GOTOPOS:
		SetEmptySelection(static_cast<int>(wParam));
		EnsureCaretVisible();
		break;

	case SCI_GETCURLINE: {
			int lineCurrentPos = pdoc->LineFromPosition(sel.MainCaret());
			int lineStart = pdoc->LineStart(lineCurrentPos);
			unsigned int lineEnd = pdoc->LineStart(lineCurrentPos + 1);
			if (lParam == 0) {
				return 1 + lineEnd - lineStart;
			}
			PLATFORM_ASSERT(wParam > 0);
			char *ptr = CharPtrFromSPtr(lParam);
			unsigned int iPlace = 0;
			for (unsigned int iChar = lineStart; iChar < lineEnd && iPlace < wParam - 1; iChar++) {
				ptr[iPlace++] = pdoc->CharAt(iChar);
			}
			ptr[iPlace] = '\0';
			return sel.MainCaret() - lineStart;
		}

	case SCI_GETENDSTYLED:
		return pdoc->GetEndStyled();

	case SCI_GETEOLMODE:
		return pdoc->eolMode;

	case SCI_SETEOLMODE:
		pdoc->eolMode = static_cast<int>(wParam);
		break;

	case SCI_SETLINEENDTYPESALLOWED:
		if (pdoc->SetLineEndTypesAllowed(static_cast<int>(wParam))) {
			cs.Clear();
			cs.InsertLines(0, pdoc->LinesTotal() - 1);
			SetAnnotationHeights(0, pdoc->LinesTotal());
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETLINEENDTYPESALLOWED:
		return pdoc->GetLineEndTypesAllowed();

	case SCI_GETLINEENDTYPESACTIVE:
		return pdoc->GetLineEndTypesActive();

	case SCI_STARTSTYLING:
		pdoc->StartStyling(static_cast<int>(wParam), static_cast<char>(lParam));
		break;

	case SCI_SETSTYLING:
		pdoc->SetStyleFor(static_cast<int>(wParam), static_cast<char>(lParam));
		break;

	case SCI_SETSTYLINGEX:             // Specify a complete styling buffer
		if (lParam == 0)
			return 0;
		pdoc->SetStyles(static_cast<int>(wParam), CharPtrFromSPtr(lParam));
		break;

	case SCI_SETBUFFEREDDRAW:
		view.bufferedDraw = wParam != 0;
		break;

	case SCI_GETBUFFEREDDRAW:
		return view.bufferedDraw;

	case SCI_GETTWOPHASEDRAW:
		return view.phasesDraw == EditView::phasesTwo;

	case SCI_SETTWOPHASEDRAW:
		if (view.SetTwoPhaseDraw(wParam != 0))
			InvalidateStyleRedraw();
		break;

	case SCI_GETPHASESDRAW:
		return view.phasesDraw;

	case SCI_SETPHASESDRAW:
		if (view.SetPhasesDraw(static_cast<int>(wParam)))
			InvalidateStyleRedraw();
		break;

	case SCI_SETFONTQUALITY:
		vs.extraFontFlag &= ~SC_EFF_QUALITY_MASK;
		vs.extraFontFlag |= (wParam & SC_EFF_QUALITY_MASK);
		InvalidateStyleRedraw();
		break;

	case SCI_GETFONTQUALITY:
		return (vs.extraFontFlag & SC_EFF_QUALITY_MASK);

	case SCI_SETTABWIDTH:
		if (wParam > 0) {
			pdoc->tabInChars = static_cast<int>(wParam);
			if (pdoc->indentInChars == 0)
				pdoc->actualIndentInChars = pdoc->tabInChars;
		}
		InvalidateStyleRedraw();
		break;

	case SCI_GETTABWIDTH:
		return pdoc->tabInChars;

	case SCI_CLEARTABSTOPS:
		if (view.ClearTabstops(static_cast<int>(wParam))) {
			DocModification mh(SC_MOD_CHANGETABSTOPS, 0, 0, 0, 0, static_cast<int>(wParam));
			NotifyModified(pdoc, mh, NULL);
		}
		break;

	case SCI_ADDTABSTOP:
		if (view.AddTabstop(static_cast<int>(wParam), static_cast<int>(lParam))) {
			DocModification mh(SC_MOD_CHANGETABSTOPS, 0, 0, 0, 0, static_cast<int>(wParam));
			NotifyModified(pdoc, mh, NULL);
		}
		break;

	case SCI_GETNEXTTABSTOP:
		return view.GetNextTabstop(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_SETINDENT:
		pdoc->indentInChars = static_cast<int>(wParam);
		if (pdoc->indentInChars != 0)
			pdoc->actualIndentInChars = pdoc->indentInChars;
		else
			pdoc->actualIndentInChars = pdoc->tabInChars;
		InvalidateStyleRedraw();
		break;

	case SCI_GETINDENT:
		return pdoc->indentInChars;

	case SCI_SETUSETABS:
		pdoc->useTabs = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETUSETABS:
		return pdoc->useTabs;

	case SCI_SETLINEINDENTATION:
		pdoc->SetLineIndentation(static_cast<int>(wParam), static_cast<int>(lParam));
		break;

	case SCI_GETLINEINDENTATION:
		return pdoc->GetLineIndentation(static_cast<int>(wParam));

	case SCI_GETLINEINDENTPOSITION:
		return pdoc->GetLineIndentPosition(static_cast<int>(wParam));

	case SCI_SETTABINDENTS:
		pdoc->tabIndents = wParam != 0;
		break;

	case SCI_GETTABINDENTS:
		return pdoc->tabIndents;

	case SCI_SETBACKSPACEUNINDENTS:
		pdoc->backspaceUnindents = wParam != 0;
		break;

	case SCI_GETBACKSPACEUNINDENTS:
		return pdoc->backspaceUnindents;

	case SCI_SETMOUSEDWELLTIME:
		dwellDelay = static_cast<int>(wParam);
		ticksToDwell = dwellDelay;
		break;

	case SCI_GETMOUSEDWELLTIME:
		return dwellDelay;

	case SCI_WORDSTARTPOSITION:
		return pdoc->ExtendWordSelect(static_cast<int>(wParam), -1, lParam != 0);

	case SCI_WORDENDPOSITION:
		return pdoc->ExtendWordSelect(static_cast<int>(wParam), 1, lParam != 0);

	case SCI_SETWRAPMODE:
		if (vs.SetWrapState(static_cast<int>(wParam))) {
			xOffset = 0;
			ContainerNeedsUpdate(SC_UPDATE_H_SCROLL);
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETWRAPMODE:
		return vs.wrapState;

	case SCI_SETWRAPVISUALFLAGS:
		if (vs.SetWrapVisualFlags(static_cast<int>(wParam))) {
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETWRAPVISUALFLAGS:
		return vs.wrapVisualFlags;

	case SCI_SETWRAPVISUALFLAGSLOCATION:
		if (vs.SetWrapVisualFlagsLocation(static_cast<int>(wParam))) {
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETWRAPVISUALFLAGSLOCATION:
		return vs.wrapVisualFlagsLocation;

	case SCI_SETWRAPSTARTINDENT:
		if (vs.SetWrapVisualStartIndent(static_cast<int>(wParam))) {
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETWRAPSTARTINDENT:
		return vs.wrapVisualStartIndent;

	case SCI_SETWRAPINDENTMODE:
		if (vs.SetWrapIndentMode(static_cast<int>(wParam))) {
			InvalidateStyleRedraw();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETWRAPINDENTMODE:
		return vs.wrapIndentMode;

	case SCI_SETLAYOUTCACHE:
		view.llc.SetLevel(static_cast<int>(wParam));
		break;

	case SCI_GETLAYOUTCACHE:
		return view.llc.GetLevel();

	case SCI_SETPOSITIONCACHE:
		view.posCache.SetSize(wParam);
		break;

	case SCI_GETPOSITIONCACHE:
		return view.posCache.GetSize();

	case SCI_SETSCROLLWIDTH:
		PLATFORM_ASSERT(wParam > 0);
		if ((wParam > 0) && (wParam != static_cast<unsigned int >(scrollWidth))) {
			view.lineWidthMaxSeen = 0;
			scrollWidth = static_cast<int>(wParam);
			SetScrollBars();
		}
		break;

	case SCI_GETSCROLLWIDTH:
		return scrollWidth;

	case SCI_SETSCROLLWIDTHTRACKING:
		trackLineWidth = wParam != 0;
		break;

	case SCI_GETSCROLLWIDTHTRACKING:
		return trackLineWidth;

	case SCI_LINESJOIN:
		LinesJoin();
		break;

	case SCI_LINESSPLIT:
		LinesSplit(static_cast<int>(wParam));
		break;

	case SCI_TEXTWIDTH:
		PLATFORM_ASSERT(wParam < vs.styles.size());
		PLATFORM_ASSERT(lParam);
		return TextWidth(static_cast<int>(wParam), CharPtrFromSPtr(lParam));

	case SCI_TEXTHEIGHT:
		return vs.lineHeight;

	case SCI_SETENDATLASTLINE:
		PLATFORM_ASSERT((wParam == 0) || (wParam == 1));
		if (endAtLastLine != (wParam != 0)) {
			endAtLastLine = wParam != 0;
			SetScrollBars();
		}
		break;

	case SCI_GETENDATLASTLINE:
		return endAtLastLine;

	case SCI_SETCARETSTICKY:
		PLATFORM_ASSERT(wParam <= SC_CARETSTICKY_WHITESPACE);
		if (wParam <= SC_CARETSTICKY_WHITESPACE) {
			caretSticky = static_cast<int>(wParam);
		}
		break;

	case SCI_GETCARETSTICKY:
		return caretSticky;

	case SCI_TOGGLECARETSTICKY:
		caretSticky = !caretSticky;
		break;

	case SCI_GETCOLUMN:
		return pdoc->GetColumn(static_cast<int>(wParam));

	case SCI_FINDCOLUMN:
		return pdoc->FindColumn(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_SETHSCROLLBAR :
		if (horizontalScrollBarVisible != (wParam != 0)) {
			horizontalScrollBarVisible = wParam != 0;
			SetScrollBars();
			ReconfigureScrollBars();
		}
		break;

	case SCI_GETHSCROLLBAR:
		return horizontalScrollBarVisible;

	case SCI_SETVSCROLLBAR:
		if (verticalScrollBarVisible != (wParam != 0)) {
			verticalScrollBarVisible = wParam != 0;
			SetScrollBars();
			ReconfigureScrollBars();
			if (verticalScrollBarVisible)
				SetVerticalScrollPos();
		}
		break;

	case SCI_GETVSCROLLBAR:
		return verticalScrollBarVisible;

	case SCI_SETINDENTATIONGUIDES:
		vs.viewIndentationGuides = IndentView(wParam);
		Redraw();
		break;

	case SCI_GETINDENTATIONGUIDES:
		return vs.viewIndentationGuides;

	case SCI_SETHIGHLIGHTGUIDE:
		if ((highlightGuideColumn != static_cast<int>(wParam)) || (wParam > 0)) {
			highlightGuideColumn = static_cast<int>(wParam);
			Redraw();
		}
		break;

	case SCI_GETHIGHLIGHTGUIDE:
		return highlightGuideColumn;

	case SCI_GETLINEENDPOSITION:
		return pdoc->LineEnd(static_cast<int>(wParam));

	case SCI_SETCODEPAGE:
		if (ValidCodePage(static_cast<int>(wParam))) {
			if (pdoc->SetDBCSCodePage(static_cast<int>(wParam))) {
				cs.Clear();
				cs.InsertLines(0, pdoc->LinesTotal() - 1);
				SetAnnotationHeights(0, pdoc->LinesTotal());
				InvalidateStyleRedraw();
				SetRepresentations();
			}
		}
		break;

	case SCI_GETCODEPAGE:
		return pdoc->dbcsCodePage;

	case SCI_SETIMEINTERACTION:
		imeInteraction = static_cast<EditModel::IMEInteraction>(wParam);
		break;

	case SCI_GETIMEINTERACTION:
		return imeInteraction;
		
#ifdef INCLUDE_DEPRECATED_FEATURES
	case SCI_SETUSEPALETTE:
		InvalidateStyleRedraw();
		break;

	case SCI_GETUSEPALETTE:
		return 0;
#endif

		// Marker definition and setting
	case SCI_MARKERDEFINE:
		if (wParam <= MARKER_MAX) {
			vs.markers[wParam].markType = static_cast<int>(lParam);
			vs.CalcLargestMarkerHeight();
		}
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case SCI_MARKERSYMBOLDEFINED:
		if (wParam <= MARKER_MAX)
			return vs.markers[wParam].markType;
		else
			return 0;

	case SCI_MARKERSETFORE:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].fore = ColourDesired(static_cast<long>(lParam));
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case SCI_MARKERSETBACKSELECTED:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].backSelected = ColourDesired(static_cast<long>(lParam));
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case SCI_MARKERENABLEHIGHLIGHT:
		marginView.highlightDelimiter.isEnabled = wParam == 1;
		RedrawSelMargin();
		break;
	case SCI_MARKERSETBACK:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].back = ColourDesired(static_cast<long>(lParam));
		InvalidateStyleData();
		RedrawSelMargin();
		break;
	case SCI_MARKERSETALPHA:
		if (wParam <= MARKER_MAX)
			vs.markers[wParam].alpha = static_cast<int>(lParam);
		InvalidateStyleRedraw();
		break;
	case SCI_MARKERADD: {
			int markerID = pdoc->AddMark(static_cast<int>(wParam), static_cast<int>(lParam));
			return markerID;
		}
	case SCI_MARKERADDSET:
		if (lParam != 0)
			pdoc->AddMarkSet(static_cast<int>(wParam), static_cast<int>(lParam));
		break;

	case SCI_MARKERDELETE:
		pdoc->DeleteMark(static_cast<int>(wParam), static_cast<int>(lParam));
		break;

	case SCI_MARKERDELETEALL:
		pdoc->DeleteAllMarks(static_cast<int>(wParam));
		break;

	case SCI_MARKERGET:
		return pdoc->GetMark(static_cast<int>(wParam));

	case SCI_MARKERNEXT:
		return pdoc->MarkerNext(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_MARKERPREVIOUS: {
			for (int iLine = static_cast<int>(wParam); iLine >= 0; iLine--) {
				if ((pdoc->GetMark(iLine) & lParam) != 0)
					return iLine;
			}
		}
		return -1;

	case SCI_MARKERDEFINEPIXMAP:
		if (wParam <= MARKER_MAX) {
			vs.markers[wParam].SetXPM(CharPtrFromSPtr(lParam));
			vs.CalcLargestMarkerHeight();
		}
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case SCI_RGBAIMAGESETWIDTH:
		sizeRGBAImage.x = static_cast<XYPOSITION>(wParam);
		break;

	case SCI_RGBAIMAGESETHEIGHT:
		sizeRGBAImage.y = static_cast<XYPOSITION>(wParam);
		break;

	case SCI_RGBAIMAGESETSCALE:
		scaleRGBAImage = static_cast<float>(wParam);
		break;

	case SCI_MARKERDEFINERGBAIMAGE:
		if (wParam <= MARKER_MAX) {
			vs.markers[wParam].SetRGBAImage(sizeRGBAImage, scaleRGBAImage / 100.0f, reinterpret_cast<unsigned char *>(lParam));
			vs.CalcLargestMarkerHeight();
		}
		InvalidateStyleData();
		RedrawSelMargin();
		break;

	case SCI_SETMARGINTYPEN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].style = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETMARGINTYPEN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].style;
		else
			return 0;

	case SCI_SETMARGINWIDTHN:
		if (ValidMargin(wParam)) {
			// Short-circuit if the width is unchanged, to avoid unnecessary redraw.
			if (vs.ms[wParam].width != lParam) {
				lastXChosen += static_cast<int>(lParam) - vs.ms[wParam].width;
				vs.ms[wParam].width = static_cast<int>(lParam);
				InvalidateStyleRedraw();
			}
		}
		break;

	case SCI_GETMARGINWIDTHN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].width;
		else
			return 0;

	case SCI_SETMARGINMASKN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].mask = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETMARGINMASKN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].mask;
		else
			return 0;

	case SCI_SETMARGINSENSITIVEN:
		if (ValidMargin(wParam)) {
			vs.ms[wParam].sensitive = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_GETMARGINSENSITIVEN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].sensitive ? 1 : 0;
		else
			return 0;

	case SCI_SETMARGINCURSORN:
		if (ValidMargin(wParam))
			vs.ms[wParam].cursor = static_cast<int>(lParam);
		break;

	case SCI_GETMARGINCURSORN:
		if (ValidMargin(wParam))
			return vs.ms[wParam].cursor;
		else
			return 0;

	case SCI_STYLECLEARALL:
		vs.ClearStyles();
		InvalidateStyleRedraw();
		break;

	case SCI_STYLESETFORE:
	case SCI_STYLESETBACK:
	case SCI_STYLESETBOLD:
	case SCI_STYLESETWEIGHT:
	case SCI_STYLESETITALIC:
	case SCI_STYLESETEOLFILLED:
	case SCI_STYLESETSIZE:
	case SCI_STYLESETSIZEFRACTIONAL:
	case SCI_STYLESETFONT:
	case SCI_STYLESETUNDERLINE:
	case SCI_STYLESETCASE:
	case SCI_STYLESETCHARACTERSET:
	case SCI_STYLESETVISIBLE:
	case SCI_STYLESETCHANGEABLE:
	case SCI_STYLESETHOTSPOT:
		StyleSetMessage(iMessage, wParam, lParam);
		break;

	case SCI_STYLEGETFORE:
	case SCI_STYLEGETBACK:
	case SCI_STYLEGETBOLD:
	case SCI_STYLEGETWEIGHT:
	case SCI_STYLEGETITALIC:
	case SCI_STYLEGETEOLFILLED:
	case SCI_STYLEGETSIZE:
	case SCI_STYLEGETSIZEFRACTIONAL:
	case SCI_STYLEGETFONT:
	case SCI_STYLEGETUNDERLINE:
	case SCI_STYLEGETCASE:
	case SCI_STYLEGETCHARACTERSET:
	case SCI_STYLEGETVISIBLE:
	case SCI_STYLEGETCHANGEABLE:
	case SCI_STYLEGETHOTSPOT:
		return StyleGetMessage(iMessage, wParam, lParam);

	case SCI_STYLERESETDEFAULT:
		vs.ResetDefaultStyle();
		InvalidateStyleRedraw();
		break;
	case SCI_SETSTYLEBITS:
		vs.EnsureStyle(0xff);
		break;

	case SCI_GETSTYLEBITS:
		return 8;

	case SCI_SETLINESTATE:
		return pdoc->SetLineState(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_GETLINESTATE:
		return pdoc->GetLineState(static_cast<int>(wParam));

	case SCI_GETMAXLINESTATE:
		return pdoc->GetMaxLineState();

	case SCI_GETCARETLINEVISIBLE:
		return vs.showCaretLineBackground;
	case SCI_SETCARETLINEVISIBLE:
		vs.showCaretLineBackground = wParam != 0;
		InvalidateStyleRedraw();
		break;
	case SCI_GETCARETLINEVISIBLEALWAYS:
		return vs.alwaysShowCaretLineBackground;
	case SCI_SETCARETLINEVISIBLEALWAYS:
		vs.alwaysShowCaretLineBackground = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETLINEBACK:
		return vs.caretLineBackground.AsLong();
	case SCI_SETCARETLINEBACK:
		vs.caretLineBackground = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;
	case SCI_GETCARETLINEBACKALPHA:
		return vs.caretLineAlpha;
	case SCI_SETCARETLINEBACKALPHA:
		vs.caretLineAlpha = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

		// Folding messages

	case SCI_VISIBLEFROMDOCLINE:
		return cs.DisplayFromDoc(static_cast<int>(wParam));

	case SCI_DOCLINEFROMVISIBLE:
		return cs.DocFromDisplay(static_cast<int>(wParam));

	case SCI_WRAPCOUNT:
		return WrapCount(static_cast<int>(wParam));

	case SCI_SETFOLDLEVEL: {
			int prev = pdoc->SetLevel(static_cast<int>(wParam), static_cast<int>(lParam));
			if (prev != static_cast<int>(lParam))
				RedrawSelMargin();
			return prev;
		}

	case SCI_GETFOLDLEVEL:
		return pdoc->GetLevel(static_cast<int>(wParam));

	case SCI_GETLASTCHILD:
		return pdoc->GetLastChild(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_GETFOLDPARENT:
		return pdoc->GetFoldParent(static_cast<int>(wParam));

	case SCI_SHOWLINES:
		cs.SetVisible(static_cast<int>(wParam), static_cast<int>(lParam), true);
		SetScrollBars();
		Redraw();
		break;

	case SCI_HIDELINES:
		if (wParam > 0)
			cs.SetVisible(static_cast<int>(wParam), static_cast<int>(lParam), false);
		SetScrollBars();
		Redraw();
		break;

	case SCI_GETLINEVISIBLE:
		return cs.GetVisible(static_cast<int>(wParam));

	case SCI_GETALLLINESVISIBLE:
		return cs.HiddenLines() ? 0 : 1;

	case SCI_SETFOLDEXPANDED:
		SetFoldExpanded(static_cast<int>(wParam), lParam != 0);
		break;

	case SCI_GETFOLDEXPANDED:
		return cs.GetExpanded(static_cast<int>(wParam));

	case SCI_SETAUTOMATICFOLD:
		foldAutomatic = static_cast<int>(wParam);
		break;

	case SCI_GETAUTOMATICFOLD:
		return foldAutomatic;

	case SCI_SETFOLDFLAGS:
		foldFlags = static_cast<int>(wParam);
		Redraw();
		break;

	case SCI_TOGGLEFOLD:
		FoldLine(static_cast<int>(wParam), SC_FOLDACTION_TOGGLE);
		break;

	case SCI_FOLDLINE:
		FoldLine(static_cast<int>(wParam), static_cast<int>(lParam));
		break;

	case SCI_FOLDCHILDREN:
		FoldExpand(static_cast<int>(wParam), static_cast<int>(lParam), pdoc->GetLevel(static_cast<int>(wParam)));
		break;

	case SCI_FOLDALL:
		FoldAll(static_cast<int>(wParam));
		break;

	case SCI_EXPANDCHILDREN:
		FoldExpand(static_cast<int>(wParam), SC_FOLDACTION_EXPAND, static_cast<int>(lParam));
		break;

	case SCI_CONTRACTEDFOLDNEXT:
		return ContractedFoldNext(static_cast<int>(wParam));

	case SCI_ENSUREVISIBLE:
		EnsureLineVisible(static_cast<int>(wParam), false);
		break;

	case SCI_ENSUREVISIBLEENFORCEPOLICY:
		EnsureLineVisible(static_cast<int>(wParam), true);
		break;

	case SCI_SCROLLRANGE:
		ScrollRange(SelectionRange(static_cast<int>(wParam), static_cast<int>(lParam)));
		break;

	case SCI_SEARCHANCHOR:
		SearchAnchor();
		break;

	case SCI_SEARCHNEXT:
	case SCI_SEARCHPREV:
		return SearchText(iMessage, wParam, lParam);

	case SCI_SETXCARETPOLICY:
		caretXPolicy = static_cast<int>(wParam);
		caretXSlop = static_cast<int>(lParam);
		break;

	case SCI_SETYCARETPOLICY:
		caretYPolicy = static_cast<int>(wParam);
		caretYSlop = static_cast<int>(lParam);
		break;

	case SCI_SETVISIBLEPOLICY:
		visiblePolicy = static_cast<int>(wParam);
		visibleSlop = static_cast<int>(lParam);
		break;

	case SCI_LINESONSCREEN:
		return LinesOnScreen();

	case SCI_SETSELFORE:
		vs.selColours.fore = ColourOptional(wParam, lParam);
		vs.selAdditionalForeground = ColourDesired(static_cast<long>(lParam));
		InvalidateStyleRedraw();
		break;

	case SCI_SETSELBACK:
		vs.selColours.back = ColourOptional(wParam, lParam);
		vs.selAdditionalBackground = ColourDesired(static_cast<long>(lParam));
		InvalidateStyleRedraw();
		break;

	case SCI_SETSELALPHA:
		vs.selAlpha = static_cast<int>(wParam);
		vs.selAdditionalAlpha = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETSELALPHA:
		return vs.selAlpha;

	case SCI_GETSELEOLFILLED:
		return vs.selEOLFilled;

	case SCI_SETSELEOLFILLED:
		vs.selEOLFilled = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_SETWHITESPACEFORE:
		vs.whitespaceColours.fore = ColourOptional(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETWHITESPACEBACK:
		vs.whitespaceColours.back = ColourOptional(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETCARETFORE:
		vs.caretcolour = ColourDesired(static_cast<long>(wParam));
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETFORE:
		return vs.caretcolour.AsLong();

	case SCI_SETCARETSTYLE:
		if (wParam <= CARETSTYLE_BLOCK)
			vs.caretStyle = static_cast<int>(wParam);
		else
			/* Default to the line caret */
			vs.caretStyle = CARETSTYLE_LINE;
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETSTYLE:
		return vs.caretStyle;

	case SCI_SETCARETWIDTH:
		if (static_cast<int>(wParam) <= 0)
			vs.caretWidth = 0;
		else if (wParam >= 3)
			vs.caretWidth = 3;
		else
			vs.caretWidth = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETCARETWIDTH:
		return vs.caretWidth;

	case SCI_ASSIGNCMDKEY:
		kmap.AssignCmdKey(Platform::LowShortFromLong(static_cast<long>(wParam)),
			Platform::HighShortFromLong(static_cast<long>(wParam)), static_cast<unsigned int>(lParam));
		break;

	case SCI_CLEARCMDKEY:
		kmap.AssignCmdKey(Platform::LowShortFromLong(static_cast<long>(wParam)),
			Platform::HighShortFromLong(static_cast<long>(wParam)), SCI_NULL);
		break;

	case SCI_CLEARALLCMDKEYS:
		kmap.Clear();
		break;

	case SCI_INDICSETSTYLE:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].sacNormal.style = static_cast<int>(lParam);
			vs.indicators[wParam].sacHover.style = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETSTYLE:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].sacNormal.style : 0;

	case SCI_INDICSETFORE:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].sacNormal.fore = ColourDesired(static_cast<long>(lParam));
			vs.indicators[wParam].sacHover.fore = ColourDesired(static_cast<long>(lParam));
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETFORE:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].sacNormal.fore.AsLong() : 0;

	case SCI_INDICSETHOVERSTYLE:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].sacHover.style = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETHOVERSTYLE:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].sacHover.style : 0;

	case SCI_INDICSETHOVERFORE:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].sacHover.fore = ColourDesired(static_cast<long>(lParam));
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETHOVERFORE:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].sacHover.fore.AsLong() : 0;

	case SCI_INDICSETFLAGS:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].SetFlags(static_cast<int>(lParam));
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETFLAGS:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].Flags() : 0;

	case SCI_INDICSETUNDER:
		if (wParam <= INDIC_MAX) {
			vs.indicators[wParam].under = lParam != 0;
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETUNDER:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].under : 0;

	case SCI_INDICSETALPHA:
		if (wParam <= INDIC_MAX && lParam >=0 && lParam <= 255) {
			vs.indicators[wParam].fillAlpha = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETALPHA:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].fillAlpha : 0;

	case SCI_INDICSETOUTLINEALPHA:
		if (wParam <= INDIC_MAX && lParam >=0 && lParam <= 255) {
			vs.indicators[wParam].outlineAlpha = static_cast<int>(lParam);
			InvalidateStyleRedraw();
		}
		break;

	case SCI_INDICGETOUTLINEALPHA:
		return (wParam <= INDIC_MAX) ? vs.indicators[wParam].outlineAlpha : 0;

	case SCI_SETINDICATORCURRENT:
		pdoc->decorations.SetCurrentIndicator(static_cast<int>(wParam));
		break;
	case SCI_GETINDICATORCURRENT:
		return pdoc->decorations.GetCurrentIndicator();
	case SCI_SETINDICATORVALUE:
		pdoc->decorations.SetCurrentValue(static_cast<int>(wParam));
		break;
	case SCI_GETINDICATORVALUE:
		return pdoc->decorations.GetCurrentValue();

	case SCI_INDICATORFILLRANGE:
		pdoc->DecorationFillRange(static_cast<int>(wParam), pdoc->decorations.GetCurrentValue(), static_cast<int>(lParam));
		break;

	case SCI_INDICATORCLEARRANGE:
		pdoc->DecorationFillRange(static_cast<int>(wParam), 0, static_cast<int>(lParam));
		break;

	case SCI_INDICATORALLONFOR:
		return pdoc->decorations.AllOnFor(static_cast<int>(wParam));

	case SCI_INDICATORVALUEAT:
		return pdoc->decorations.ValueAt(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_INDICATORSTART:
		return pdoc->decorations.Start(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_INDICATOREND:
		return pdoc->decorations.End(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_LINEDOWN:
	case SCI_LINEDOWNEXTEND:
	case SCI_PARADOWN:
	case SCI_PARADOWNEXTEND:
	case SCI_LINEUP:
	case SCI_LINEUPEXTEND:
	case SCI_PARAUP:
	case SCI_PARAUPEXTEND:
	case SCI_CHARLEFT:
	case SCI_CHARLEFTEXTEND:
	case SCI_CHARRIGHT:
	case SCI_CHARRIGHTEXTEND:
	case SCI_WORDLEFT:
	case SCI_WORDLEFTEXTEND:
	case SCI_WORDRIGHT:
	case SCI_WORDRIGHTEXTEND:
	case SCI_WORDLEFTEND:
	case SCI_WORDLEFTENDEXTEND:
	case SCI_WORDRIGHTEND:
	case SCI_WORDRIGHTENDEXTEND:
	case SCI_HOME:
	case SCI_HOMEEXTEND:
	case SCI_LINEEND:
	case SCI_LINEENDEXTEND:
	case SCI_HOMEWRAP:
	case SCI_HOMEWRAPEXTEND:
	case SCI_LINEENDWRAP:
	case SCI_LINEENDWRAPEXTEND:
	case SCI_DOCUMENTSTART:
	case SCI_DOCUMENTSTARTEXTEND:
	case SCI_DOCUMENTEND:
	case SCI_DOCUMENTENDEXTEND:
	case SCI_SCROLLTOSTART:
	case SCI_SCROLLTOEND:

	case SCI_STUTTEREDPAGEUP:
	case SCI_STUTTEREDPAGEUPEXTEND:
	case SCI_STUTTEREDPAGEDOWN:
	case SCI_STUTTEREDPAGEDOWNEXTEND:

	case SCI_PAGEUP:
	case SCI_PAGEUPEXTEND:
	case SCI_PAGEDOWN:
	case SCI_PAGEDOWNEXTEND:
	case SCI_EDITTOGGLEOVERTYPE:
	case SCI_CANCEL:
	case SCI_DELETEBACK:
	case SCI_TAB:
	case SCI_BACKTAB:
	case SCI_NEWLINE:
	case SCI_FORMFEED:
	case SCI_VCHOME:
	case SCI_VCHOMEEXTEND:
	case SCI_VCHOMEWRAP:
	case SCI_VCHOMEWRAPEXTEND:
	case SCI_VCHOMEDISPLAY:
	case SCI_VCHOMEDISPLAYEXTEND:
	case SCI_ZOOMIN:
	case SCI_ZOOMOUT:
	case SCI_DELWORDLEFT:
	case SCI_DELWORDRIGHT:
	case SCI_DELWORDRIGHTEND:
	case SCI_DELLINELEFT:
	case SCI_DELLINERIGHT:
	case SCI_LINECOPY:
	case SCI_LINECUT:
	case SCI_LINEDELETE:
	case SCI_LINETRANSPOSE:
	case SCI_LINEDUPLICATE:
	case SCI_LOWERCASE:
	case SCI_UPPERCASE:
	case SCI_LINESCROLLDOWN:
	case SCI_LINESCROLLUP:
	case SCI_WORDPARTLEFT:
	case SCI_WORDPARTLEFTEXTEND:
	case SCI_WORDPARTRIGHT:
	case SCI_WORDPARTRIGHTEXTEND:
	case SCI_DELETEBACKNOTLINE:
	case SCI_HOMEDISPLAY:
	case SCI_HOMEDISPLAYEXTEND:
	case SCI_LINEENDDISPLAY:
	case SCI_LINEENDDISPLAYEXTEND:
	case SCI_LINEDOWNRECTEXTEND:
	case SCI_LINEUPRECTEXTEND:
	case SCI_CHARLEFTRECTEXTEND:
	case SCI_CHARRIGHTRECTEXTEND:
	case SCI_HOMERECTEXTEND:
	case SCI_VCHOMERECTEXTEND:
	case SCI_LINEENDRECTEXTEND:
	case SCI_PAGEUPRECTEXTEND:
	case SCI_PAGEDOWNRECTEXTEND:
	case SCI_SELECTIONDUPLICATE:
		return KeyCommand(iMessage);

	case SCI_BRACEHIGHLIGHT:
		SetBraceHighlight(static_cast<int>(wParam), static_cast<int>(lParam), STYLE_BRACELIGHT);
		break;

	case SCI_BRACEHIGHLIGHTINDICATOR:
		if (lParam >= 0 && lParam <= INDIC_MAX) {
			vs.braceHighlightIndicatorSet = wParam != 0;
			vs.braceHighlightIndicator = static_cast<int>(lParam);
		}
		break;

	case SCI_BRACEBADLIGHT:
		SetBraceHighlight(static_cast<int>(wParam), -1, STYLE_BRACEBAD);
		break;

	case SCI_BRACEBADLIGHTINDICATOR:
		if (lParam >= 0 && lParam <= INDIC_MAX) {
			vs.braceBadLightIndicatorSet = wParam != 0;
			vs.braceBadLightIndicator = static_cast<int>(lParam);
		}
		break;

	case SCI_BRACEMATCH:
		// wParam is position of char to find brace for,
		// lParam is maximum amount of text to restyle to find it
		return pdoc->BraceMatch(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_GETVIEWEOL:
		return vs.viewEOL;

	case SCI_SETVIEWEOL:
		vs.viewEOL = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_SETZOOM:
		vs.zoomLevel = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		NotifyZoom();
		break;

	case SCI_GETZOOM:
		return vs.zoomLevel;

	case SCI_GETEDGECOLUMN:
		return vs.theEdge;

	case SCI_SETEDGECOLUMN:
		vs.theEdge = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETEDGEMODE:
		return vs.edgeState;

	case SCI_SETEDGEMODE:
		vs.edgeState = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETEDGECOLOUR:
		return vs.edgecolour.AsLong();

	case SCI_SETEDGECOLOUR:
		vs.edgecolour = ColourDesired(static_cast<long>(wParam));
		InvalidateStyleRedraw();
		break;

	case SCI_GETDOCPOINTER:
		return reinterpret_cast<sptr_t>(pdoc);

	case SCI_SETDOCPOINTER:
		CancelModes();
		SetDocPointer(reinterpret_cast<Document *>(lParam));
		return 0;

	case SCI_CREATEDOCUMENT: {
			Document *doc = new Document();
			doc->AddRef();
			return reinterpret_cast<sptr_t>(doc);
		}

	case SCI_ADDREFDOCUMENT:
		(reinterpret_cast<Document *>(lParam))->AddRef();
		break;

	case SCI_RELEASEDOCUMENT:
		(reinterpret_cast<Document *>(lParam))->Release();
		break;

	case SCI_CREATELOADER: {
			Document *doc = new Document();
			doc->AddRef();
			doc->Allocate(static_cast<int>(wParam));
			doc->SetUndoCollection(false);
			return reinterpret_cast<sptr_t>(static_cast<ILoader *>(doc));
		}

	case SCI_SETMODEVENTMASK:
		modEventMask = static_cast<int>(wParam);
		return 0;

	case SCI_GETMODEVENTMASK:
		return modEventMask;

	case SCI_CONVERTEOLS:
		pdoc->ConvertLineEnds(static_cast<int>(wParam));
		SetSelection(sel.MainCaret(), sel.MainAnchor());	// Ensure selection inside document
		return 0;

	case SCI_SETLENGTHFORENCODE:
		lengthForEncode = static_cast<int>(wParam);
		return 0;

	case SCI_SELECTIONISRECTANGLE:
		return sel.selType == Selection::selRectangle ? 1 : 0;

	case SCI_SETSELECTIONMODE: {
			switch (wParam) {
			case SC_SEL_STREAM:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selStream));
				sel.selType = Selection::selStream;
				break;
			case SC_SEL_RECTANGLE:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selRectangle));
				sel.selType = Selection::selRectangle;
				break;
			case SC_SEL_LINES:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selLines));
				sel.selType = Selection::selLines;
				break;
			case SC_SEL_THIN:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selThin));
				sel.selType = Selection::selThin;
				break;
			default:
				sel.SetMoveExtends(!sel.MoveExtends() || (sel.selType != Selection::selStream));
				sel.selType = Selection::selStream;
			}
			InvalidateSelection(sel.RangeMain(), true);
			break;
		}
	case SCI_GETSELECTIONMODE:
		switch (sel.selType) {
		case Selection::selStream:
			return SC_SEL_STREAM;
		case Selection::selRectangle:
			return SC_SEL_RECTANGLE;
		case Selection::selLines:
			return SC_SEL_LINES;
		case Selection::selThin:
			return SC_SEL_THIN;
		default:	// ?!
			return SC_SEL_STREAM;
		}
	case SCI_GETLINESELSTARTPOSITION:
	case SCI_GETLINESELENDPOSITION: {
			SelectionSegment segmentLine(SelectionPosition(pdoc->LineStart(static_cast<int>(wParam))),
				SelectionPosition(pdoc->LineEnd(static_cast<int>(wParam))));
			for (size_t r=0; r<sel.Count(); r++) {
				SelectionSegment portion = sel.Range(r).Intersect(segmentLine);
				if (portion.start.IsValid()) {
					return (iMessage == SCI_GETLINESELSTARTPOSITION) ? portion.start.Position() : portion.end.Position();
				}
			}
			return INVALID_POSITION;
		}

	case SCI_SETOVERTYPE:
		inOverstrike = wParam != 0;
		break;

	case SCI_GETOVERTYPE:
		return inOverstrike ? 1 : 0;

	case SCI_SETFOCUS:
		SetFocusState(wParam != 0);
		break;

	case SCI_GETFOCUS:
		return hasFocus;

	case SCI_SETSTATUS:
		errorStatus = static_cast<int>(wParam);
		break;

	case SCI_GETSTATUS:
		return errorStatus;

	case SCI_SETMOUSEDOWNCAPTURES:
		mouseDownCaptures = wParam != 0;
		break;

	case SCI_GETMOUSEDOWNCAPTURES:
		return mouseDownCaptures;

	case SCI_SETCURSOR:
		cursorMode = static_cast<int>(wParam);
		DisplayCursor(Window::cursorText);
		break;

	case SCI_GETCURSOR:
		return cursorMode;

	case SCI_SETCONTROLCHARSYMBOL:
		vs.controlCharSymbol = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETCONTROLCHARSYMBOL:
		return vs.controlCharSymbol;

	case SCI_SETREPRESENTATION:
		reprs.SetRepresentation(reinterpret_cast<const char *>(wParam), CharPtrFromSPtr(lParam));
		break;

	case SCI_GETREPRESENTATION: {
			const Representation *repr = reprs.RepresentationFromCharacter(
				reinterpret_cast<const char *>(wParam), UTF8MaxBytes);
			if (repr) {
				return StringResult(lParam, repr->stringRep.c_str());
			}
			return 0;
		}

	case SCI_CLEARREPRESENTATION:
		reprs.ClearRepresentation(reinterpret_cast<const char *>(wParam));
		break;

	case SCI_STARTRECORD:
		recordingMacro = true;
		return 0;

	case SCI_STOPRECORD:
		recordingMacro = false;
		return 0;

	case SCI_MOVECARETINSIDEVIEW:
		MoveCaretInsideView();
		break;

	case SCI_SETFOLDMARGINCOLOUR:
		vs.foldmarginColour = ColourOptional(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETFOLDMARGINHICOLOUR:
		vs.foldmarginHighlightColour = ColourOptional(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_SETHOTSPOTACTIVEFORE:
		vs.hotspotColours.fore = ColourOptional(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTACTIVEFORE:
		return vs.hotspotColours.fore.AsLong();

	case SCI_SETHOTSPOTACTIVEBACK:
		vs.hotspotColours.back = ColourOptional(wParam, lParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTACTIVEBACK:
		return vs.hotspotColours.back.AsLong();

	case SCI_SETHOTSPOTACTIVEUNDERLINE:
		vs.hotspotUnderline = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTACTIVEUNDERLINE:
		return vs.hotspotUnderline ? 1 : 0;

	case SCI_SETHOTSPOTSINGLELINE:
		vs.hotspotSingleLine = wParam != 0;
		InvalidateStyleRedraw();
		break;

	case SCI_GETHOTSPOTSINGLELINE:
		return vs.hotspotSingleLine ? 1 : 0;

	case SCI_SETPASTECONVERTENDINGS:
		convertPastes = wParam != 0;
		break;

	case SCI_GETPASTECONVERTENDINGS:
		return convertPastes ? 1 : 0;

	case SCI_GETCHARACTERPOINTER:
		return reinterpret_cast<sptr_t>(pdoc->BufferPointer());

	case SCI_GETRANGEPOINTER:
		return reinterpret_cast<sptr_t>(pdoc->RangePointer(static_cast<int>(wParam), static_cast<int>(lParam)));

	case SCI_GETGAPPOSITION:
		return pdoc->GapPosition();

	case SCI_SETEXTRAASCENT:
		vs.extraAscent = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETEXTRAASCENT:
		return vs.extraAscent;

	case SCI_SETEXTRADESCENT:
		vs.extraDescent = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETEXTRADESCENT:
		return vs.extraDescent;

	case SCI_MARGINSETSTYLEOFFSET:
		vs.marginStyleOffset = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_MARGINGETSTYLEOFFSET:
		return vs.marginStyleOffset;

	case SCI_SETMARGINOPTIONS:
		marginOptions = static_cast<int>(wParam);
		break;

	case SCI_GETMARGINOPTIONS:
		return marginOptions;

	case SCI_MARGINSETTEXT:
		pdoc->MarginSetText(static_cast<int>(wParam), CharPtrFromSPtr(lParam));
		break;

	case SCI_MARGINGETTEXT: {
			const StyledText st = pdoc->MarginStyledText(static_cast<int>(wParam));
			return BytesResult(lParam, reinterpret_cast<const unsigned char *>(st.text), st.length);
		}

	case SCI_MARGINSETSTYLE:
		pdoc->MarginSetStyle(static_cast<int>(wParam), static_cast<int>(lParam));
		break;

	case SCI_MARGINGETSTYLE: {
			const StyledText st = pdoc->MarginStyledText(static_cast<int>(wParam));
			return st.style;
		}

	case SCI_MARGINSETSTYLES:
		pdoc->MarginSetStyles(static_cast<int>(wParam), reinterpret_cast<const unsigned char *>(lParam));
		break;

	case SCI_MARGINGETSTYLES: {
			const StyledText st = pdoc->MarginStyledText(static_cast<int>(wParam));
			return BytesResult(lParam, st.styles, st.length);
		}

	case SCI_MARGINTEXTCLEARALL:
		pdoc->MarginClearAll();
		break;

	case SCI_ANNOTATIONSETTEXT:
		pdoc->AnnotationSetText(static_cast<int>(wParam), CharPtrFromSPtr(lParam));
		break;

	case SCI_ANNOTATIONGETTEXT: {
			const StyledText st = pdoc->AnnotationStyledText(static_cast<int>(wParam));
			return BytesResult(lParam, reinterpret_cast<const unsigned char *>(st.text), st.length);
		}

	case SCI_ANNOTATIONGETSTYLE: {
			const StyledText st = pdoc->AnnotationStyledText(static_cast<int>(wParam));
			return st.style;
		}

	case SCI_ANNOTATIONSETSTYLE:
		pdoc->AnnotationSetStyle(static_cast<int>(wParam), static_cast<int>(lParam));
		break;

	case SCI_ANNOTATIONSETSTYLES:
		pdoc->AnnotationSetStyles(static_cast<int>(wParam), reinterpret_cast<const unsigned char *>(lParam));
		break;

	case SCI_ANNOTATIONGETSTYLES: {
			const StyledText st = pdoc->AnnotationStyledText(static_cast<int>(wParam));
			return BytesResult(lParam, st.styles, st.length);
		}

	case SCI_ANNOTATIONGETLINES:
		return pdoc->AnnotationLines(static_cast<int>(wParam));

	case SCI_ANNOTATIONCLEARALL:
		pdoc->AnnotationClearAll();
		break;

	case SCI_ANNOTATIONSETVISIBLE:
		SetAnnotationVisible(static_cast<int>(wParam));
		break;

	case SCI_ANNOTATIONGETVISIBLE:
		return vs.annotationVisible;

	case SCI_ANNOTATIONSETSTYLEOFFSET:
		vs.annotationStyleOffset = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_ANNOTATIONGETSTYLEOFFSET:
		return vs.annotationStyleOffset;

	case SCI_RELEASEALLEXTENDEDSTYLES:
		vs.ReleaseAllExtendedStyles();
		break;

	case SCI_ALLOCATEEXTENDEDSTYLES:
		return vs.AllocateExtendedStyles(static_cast<int>(wParam));

	case SCI_ADDUNDOACTION:
		pdoc->AddUndoAction(static_cast<int>(wParam), lParam & UNDO_MAY_COALESCE);
		break;

	case SCI_SETMOUSESELECTIONRECTANGULARSWITCH:
		mouseSelectionRectangularSwitch = wParam != 0;
		break;

	case SCI_GETMOUSESELECTIONRECTANGULARSWITCH:
		return mouseSelectionRectangularSwitch;

	case SCI_SETMULTIPLESELECTION:
		multipleSelection = wParam != 0;
		InvalidateCaret();
		break;

	case SCI_GETMULTIPLESELECTION:
		return multipleSelection;

	case SCI_SETADDITIONALSELECTIONTYPING:
		additionalSelectionTyping = wParam != 0;
		InvalidateCaret();
		break;

	case SCI_GETADDITIONALSELECTIONTYPING:
		return additionalSelectionTyping;

	case SCI_SETMULTIPASTE:
		multiPasteMode = static_cast<int>(wParam);
		break;

	case SCI_GETMULTIPASTE:
		return multiPasteMode;

	case SCI_SETADDITIONALCARETSBLINK:
		view.additionalCaretsBlink = wParam != 0;
		InvalidateCaret();
		break;

	case SCI_GETADDITIONALCARETSBLINK:
		return view.additionalCaretsBlink;

	case SCI_SETADDITIONALCARETSVISIBLE:
		view.additionalCaretsVisible = wParam != 0;
		InvalidateCaret();
		break;

	case SCI_GETADDITIONALCARETSVISIBLE:
		return view.additionalCaretsVisible;

	case SCI_GETSELECTIONS:
		return sel.Count();

	case SCI_GETSELECTIONEMPTY:
		return sel.Empty();

	case SCI_CLEARSELECTIONS:
		sel.Clear();
		Redraw();
		break;

	case SCI_SETSELECTION:
		sel.SetSelection(SelectionRange(static_cast<int>(wParam), static_cast<int>(lParam)));
		Redraw();
		break;

	case SCI_ADDSELECTION:
		sel.AddSelection(SelectionRange(static_cast<int>(wParam), static_cast<int>(lParam)));
		Redraw();
		break;

	case SCI_DROPSELECTIONN:
		sel.DropSelection(static_cast<int>(wParam));
		Redraw();
		break;

	case SCI_SETMAINSELECTION:
		sel.SetMain(static_cast<int>(wParam));
		Redraw();
		break;

	case SCI_GETMAINSELECTION:
		return sel.Main();

	case SCI_SETSELECTIONNCARET:
		sel.Range(wParam).caret.SetPosition(static_cast<int>(lParam));
		Redraw();
		break;

	case SCI_GETSELECTIONNCARET:
		return sel.Range(wParam).caret.Position();

	case SCI_SETSELECTIONNANCHOR:
		sel.Range(wParam).anchor.SetPosition(static_cast<int>(lParam));
		Redraw();
		break;
	case SCI_GETSELECTIONNANCHOR:
		return sel.Range(wParam).anchor.Position();

	case SCI_SETSELECTIONNCARETVIRTUALSPACE:
		sel.Range(wParam).caret.SetVirtualSpace(static_cast<int>(lParam));
		Redraw();
		break;

	case SCI_GETSELECTIONNCARETVIRTUALSPACE:
		return sel.Range(wParam).caret.VirtualSpace();

	case SCI_SETSELECTIONNANCHORVIRTUALSPACE:
		sel.Range(wParam).anchor.SetVirtualSpace(static_cast<int>(lParam));
		Redraw();
		break;

	case SCI_GETSELECTIONNANCHORVIRTUALSPACE:
		return sel.Range(wParam).anchor.VirtualSpace();

	case SCI_SETSELECTIONNSTART:
		sel.Range(wParam).anchor.SetPosition(static_cast<int>(lParam));
		Redraw();
		break;

	case SCI_GETSELECTIONNSTART:
		return sel.Range(wParam).Start().Position();

	case SCI_SETSELECTIONNEND:
		sel.Range(wParam).caret.SetPosition(static_cast<int>(lParam));
		Redraw();
		break;

	case SCI_GETSELECTIONNEND:
		return sel.Range(wParam).End().Position();

	case SCI_SETRECTANGULARSELECTIONCARET:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::selRectangle;
		sel.Rectangular().caret.SetPosition(static_cast<int>(wParam));
		SetRectangularRange();
		Redraw();
		break;

	case SCI_GETRECTANGULARSELECTIONCARET:
		return sel.Rectangular().caret.Position();

	case SCI_SETRECTANGULARSELECTIONANCHOR:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::selRectangle;
		sel.Rectangular().anchor.SetPosition(static_cast<int>(wParam));
		SetRectangularRange();
		Redraw();
		break;

	case SCI_GETRECTANGULARSELECTIONANCHOR:
		return sel.Rectangular().anchor.Position();

	case SCI_SETRECTANGULARSELECTIONCARETVIRTUALSPACE:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::selRectangle;
		sel.Rectangular().caret.SetVirtualSpace(static_cast<int>(wParam));
		SetRectangularRange();
		Redraw();
		break;

	case SCI_GETRECTANGULARSELECTIONCARETVIRTUALSPACE:
		return sel.Rectangular().caret.VirtualSpace();

	case SCI_SETRECTANGULARSELECTIONANCHORVIRTUALSPACE:
		if (!sel.IsRectangular())
			sel.Clear();
		sel.selType = Selection::selRectangle;
		sel.Rectangular().anchor.SetVirtualSpace(static_cast<int>(wParam));
		SetRectangularRange();
		Redraw();
		break;

	case SCI_GETRECTANGULARSELECTIONANCHORVIRTUALSPACE:
		return sel.Rectangular().anchor.VirtualSpace();

	case SCI_SETVIRTUALSPACEOPTIONS:
		virtualSpaceOptions = static_cast<int>(wParam);
		break;

	case SCI_GETVIRTUALSPACEOPTIONS:
		return virtualSpaceOptions;

	case SCI_SETADDITIONALSELFORE:
		vs.selAdditionalForeground = ColourDesired(static_cast<long>(wParam));
		InvalidateStyleRedraw();
		break;

	case SCI_SETADDITIONALSELBACK:
		vs.selAdditionalBackground = ColourDesired(static_cast<long>(wParam));
		InvalidateStyleRedraw();
		break;

	case SCI_SETADDITIONALSELALPHA:
		vs.selAdditionalAlpha = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_GETADDITIONALSELALPHA:
		return vs.selAdditionalAlpha;

	case SCI_SETADDITIONALCARETFORE:
		vs.additionalCaretColour = ColourDesired(static_cast<long>(wParam));
		InvalidateStyleRedraw();
		break;

	case SCI_GETADDITIONALCARETFORE:
		return vs.additionalCaretColour.AsLong();

	case SCI_ROTATESELECTION:
		sel.RotateMain();
		InvalidateSelection(sel.RangeMain(), true);
		break;

	case SCI_SWAPMAINANCHORCARET:
		InvalidateSelection(sel.RangeMain());
		sel.RangeMain() = SelectionRange(sel.RangeMain().anchor, sel.RangeMain().caret);
		break;

	case SCI_CHANGELEXERSTATE:
		pdoc->ChangeLexerState(static_cast<int>(wParam), static_cast<int>(lParam));
		break;

	case SCI_SETIDENTIFIER:
		SetCtrlID(static_cast<int>(wParam));
		break;

	case SCI_GETIDENTIFIER:
		return GetCtrlID();

	case SCI_SETTECHNOLOGY:
		// No action by default
		break;

	case SCI_GETTECHNOLOGY:
		return technology;

	case SCI_COUNTCHARACTERS:
		return pdoc->CountCharacters(static_cast<int>(wParam), static_cast<int>(lParam));

	default:
		return DefWndProc(iMessage, wParam, lParam);
	}
	//Platform::DebugPrintf("end wnd proc\n");
	return 0l;
}
