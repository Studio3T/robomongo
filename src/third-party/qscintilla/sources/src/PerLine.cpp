// Scintilla source code edit control
/** @file PerLine.cxx
 ** Manages data associated with each line of the document
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>

#include <vector>
#include <algorithm>

#include "Platform.h"

#include "Scintilla.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "CellBuffer.h"
#include "PerLine.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

MarkerHandleSet::MarkerHandleSet() {
	root = 0;
}

MarkerHandleSet::~MarkerHandleSet() {
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		MarkerHandleNumber *mhnToFree = mhn;
		mhn = mhn->next;
		delete mhnToFree;
	}
	root = 0;
}

int MarkerHandleSet::Length() const {
	int c = 0;
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		c++;
		mhn = mhn->next;
	}
	return c;
}

int MarkerHandleSet::MarkValue() const {
	unsigned int m = 0;
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		m |= (1 << mhn->number);
		mhn = mhn->next;
	}
	return m;
}

bool MarkerHandleSet::Contains(int handle) const {
	MarkerHandleNumber *mhn = root;
	while (mhn) {
		if (mhn->handle == handle) {
			return true;
		}
		mhn = mhn->next;
	}
	return false;
}

bool MarkerHandleSet::InsertHandle(int handle, int markerNum) {
	MarkerHandleNumber *mhn = new MarkerHandleNumber;
	mhn->handle = handle;
	mhn->number = markerNum;
	mhn->next = root;
	root = mhn;
	return true;
}

void MarkerHandleSet::RemoveHandle(int handle) {
	MarkerHandleNumber **pmhn = &root;
	while (*pmhn) {
		MarkerHandleNumber *mhn = *pmhn;
		if (mhn->handle == handle) {
			*pmhn = mhn->next;
			delete mhn;
			return;
		}
		pmhn = &((*pmhn)->next);
	}
}

bool MarkerHandleSet::RemoveNumber(int markerNum, bool all) {
	bool performedDeletion = false;
	MarkerHandleNumber **pmhn = &root;
	while (*pmhn) {
		MarkerHandleNumber *mhn = *pmhn;
		if (mhn->number == markerNum) {
			*pmhn = mhn->next;
			delete mhn;
			performedDeletion = true;
			if (!all)
				break;
		} else {
			pmhn = &((*pmhn)->next);
		}
	}
	return performedDeletion;
}

void MarkerHandleSet::CombineWith(MarkerHandleSet *other) {
	MarkerHandleNumber **pmhn = &root;
	while (*pmhn) {
		pmhn = &((*pmhn)->next);
	}
	*pmhn = other->root;
	other->root = 0;
}

LineMarkers::~LineMarkers() {
	Init();
}

void LineMarkers::Init() {
	for (int line = 0; line < markers.Length(); line++) {
		delete markers[line];
		markers[line] = 0;
	}
	markers.DeleteAll();
}

void LineMarkers::InsertLine(int line) {
	if (markers.Length()) {
		markers.Insert(line, 0);
	}
}

void LineMarkers::RemoveLine(int line) {
	// Retain the markers from the deleted line by oring them into the previous line
	if (markers.Length()) {
		if (line > 0) {
			MergeMarkers(line - 1);
		}
		markers.Delete(line);
	}
}

int LineMarkers::LineFromHandle(int markerHandle) {
	if (markers.Length()) {
		for (int line = 0; line < markers.Length(); line++) {
			if (markers[line]) {
				if (markers[line]->Contains(markerHandle)) {
					return line;
				}
			}
		}
	}
	return -1;
}

void LineMarkers::MergeMarkers(int pos) {
	if (markers[pos + 1] != NULL) {
		if (markers[pos] == NULL)
			markers[pos] = new MarkerHandleSet;
		markers[pos]->CombineWith(markers[pos + 1]);
		delete markers[pos + 1];
		markers[pos + 1] = NULL;
	}
}

int LineMarkers::MarkValue(int line) {
	if (markers.Length() && (line >= 0) && (line < markers.Length()) && markers[line])
		return markers[line]->MarkValue();
	else
		return 0;
}

int LineMarkers::MarkerNext(int lineStart, int mask) const {
	if (lineStart < 0)
		lineStart = 0;
	int length = markers.Length();
	for (int iLine = lineStart; iLine < length; iLine++) {
		MarkerHandleSet *onLine = markers[iLine];
		if (onLine && ((onLine->MarkValue() & mask) != 0))
		//if ((pdoc->GetMark(iLine) & lParam) != 0)
			return iLine;
	}
	return -1;
}

int LineMarkers::AddMark(int line, int markerNum, int lines) {
	handleCurrent++;
	if (!markers.Length()) {
		// No existing markers so allocate one element per line
		markers.InsertValue(0, lines, 0);
	}
	if (line >= markers.Length()) {
		return -1;
	}
	if (!markers[line]) {
		// Need new structure to hold marker handle
		markers[line] = new MarkerHandleSet();
	}
	markers[line]->InsertHandle(handleCurrent, markerNum);

	return handleCurrent;
}

bool LineMarkers::DeleteMark(int line, int markerNum, bool all) {
	bool someChanges = false;
	if (markers.Length() && (line >= 0) && (line < markers.Length()) && markers[line]) {
		if (markerNum == -1) {
			someChanges = true;
			delete markers[line];
			markers[line] = NULL;
		} else {
			someChanges = markers[line]->RemoveNumber(markerNum, all);
			if (markers[line]->Length() == 0) {
				delete markers[line];
				markers[line] = NULL;
			}
		}
	}
	return someChanges;
}

void LineMarkers::DeleteMarkFromHandle(int markerHandle) {
	int line = LineFromHandle(markerHandle);
	if (line >= 0) {
		markers[line]->RemoveHandle(markerHandle);
		if (markers[line]->Length() == 0) {
			delete markers[line];
			markers[line] = NULL;
		}
	}
}

LineLevels::~LineLevels() {
}

void LineLevels::Init() {
	levels.DeleteAll();
}

void LineLevels::InsertLine(int line) {
	if (levels.Length()) {
		int level = (line < levels.Length()) ? levels[line] : SC_FOLDLEVELBASE;
		levels.InsertValue(line, 1, level);
	}
}

void LineLevels::RemoveLine(int line) {
	if (levels.Length()) {
		// Move up following lines but merge header flag from this line
		// to line before to avoid a temporary disappearence causing expansion.
		int firstHeader = levels[line] & SC_FOLDLEVELHEADERFLAG;
		levels.Delete(line);
		if (line == levels.Length()-1) // Last line loses the header flag
			levels[line-1] &= ~SC_FOLDLEVELHEADERFLAG;
		else if (line > 0)
			levels[line-1] |= firstHeader;
	}
}

void LineLevels::ExpandLevels(int sizeNew) {
	levels.InsertValue(levels.Length(), sizeNew - levels.Length(), SC_FOLDLEVELBASE);
}

void LineLevels::ClearLevels() {
	levels.DeleteAll();
}

int LineLevels::SetLevel(int line, int level, int lines) {
	int prev = 0;
	if ((line >= 0) && (line < lines)) {
		if (!levels.Length()) {
			ExpandLevels(lines + 1);
		}
		prev = levels[line];
		if (prev != level) {
			levels[line] = level;
		}
	}
	return prev;
}

int LineLevels::GetLevel(int line) const {
	if (levels.Length() && (line >= 0) && (line < levels.Length())) {
		return levels[line];
	} else {
		return SC_FOLDLEVELBASE;
	}
}

LineState::~LineState() {
}

void LineState::Init() {
	lineStates.DeleteAll();
}

void LineState::InsertLine(int line) {
	if (lineStates.Length()) {
		lineStates.EnsureLength(line);
		int val = (line < lineStates.Length()) ? lineStates[line] : 0;
		lineStates.Insert(line, val);
	}
}

void LineState::RemoveLine(int line) {
	if (lineStates.Length() > line) {
		lineStates.Delete(line);
	}
}

int LineState::SetLineState(int line, int state) {
	lineStates.EnsureLength(line + 1);
	int stateOld = lineStates[line];
	lineStates[line] = state;
	return stateOld;
}

int LineState::GetLineState(int line) {
	if (line < 0)
		return 0;
	lineStates.EnsureLength(line + 1);
	return lineStates[line];
}

int LineState::GetMaxLineState() const {
	return lineStates.Length();
}

static int NumberLines(const char *text) {
	if (text) {
		int newLines = 0;
		while (*text) {
			if (*text == '\n')
				newLines++;
			text++;
		}
		return newLines+1;
	} else {
		return 0;
	}
}

// Each allocated LineAnnotation is a char array which starts with an AnnotationHeader
// and then has text and optional styles.

static const int IndividualStyles = 0x100;

struct AnnotationHeader {
	short style;	// Style IndividualStyles implies array of styles
	short lines;
	int length;
};

LineAnnotation::~LineAnnotation() {
	ClearAll();
}

void LineAnnotation::Init() {
	ClearAll();
}

void LineAnnotation::InsertLine(int line) {
	if (annotations.Length()) {
		annotations.EnsureLength(line);
		annotations.Insert(line, 0);
	}
}

void LineAnnotation::RemoveLine(int line) {
	if (annotations.Length() && (line > 0) && (line <= annotations.Length())) {
		delete []annotations[line-1];
		annotations.Delete(line-1);
	}
}

bool LineAnnotation::MultipleStyles(int line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return reinterpret_cast<AnnotationHeader *>(annotations[line])->style == IndividualStyles;
	else
		return 0;
}

int LineAnnotation::Style(int line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return reinterpret_cast<AnnotationHeader *>(annotations[line])->style;
	else
		return 0;
}

const char *LineAnnotation::Text(int line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return annotations[line]+sizeof(AnnotationHeader);
	else
		return 0;
}

const unsigned char *LineAnnotation::Styles(int line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line] && MultipleStyles(line))
		return reinterpret_cast<unsigned char *>(annotations[line] + sizeof(AnnotationHeader) + Length(line));
	else
		return 0;
}

static char *AllocateAnnotation(int length, int style) {
	size_t len = sizeof(AnnotationHeader) + length + ((style == IndividualStyles) ? length : 0);
	char *ret = new char[len];
	memset(ret, 0, len);
	return ret;
}

void LineAnnotation::SetText(int line, const char *text) {
	if (text && (line >= 0)) {
		annotations.EnsureLength(line+1);
		int style = Style(line);
		if (annotations[line]) {
			delete []annotations[line];
		}
		annotations[line] = AllocateAnnotation(static_cast<int>(strlen(text)), style);
		AnnotationHeader *pah = reinterpret_cast<AnnotationHeader *>(annotations[line]);
		pah->style = static_cast<short>(style);
		pah->length = static_cast<int>(strlen(text));
		pah->lines = static_cast<short>(NumberLines(text));
		memcpy(annotations[line]+sizeof(AnnotationHeader), text, pah->length);
	} else {
		if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line]) {
			delete []annotations[line];
			annotations[line] = 0;
		}
	}
}

void LineAnnotation::ClearAll() {
	for (int line = 0; line < annotations.Length(); line++) {
		delete []annotations[line];
		annotations[line] = 0;
	}
	annotations.DeleteAll();
}

void LineAnnotation::SetStyle(int line, int style) {
	annotations.EnsureLength(line+1);
	if (!annotations[line]) {
		annotations[line] = AllocateAnnotation(0, style);
	}
	reinterpret_cast<AnnotationHeader *>(annotations[line])->style = static_cast<short>(style);
}

void LineAnnotation::SetStyles(int line, const unsigned char *styles) {
	if (line >= 0) {
		annotations.EnsureLength(line+1);
		if (!annotations[line]) {
			annotations[line] = AllocateAnnotation(0, IndividualStyles);
		} else {
			AnnotationHeader *pahSource = reinterpret_cast<AnnotationHeader *>(annotations[line]);
			if (pahSource->style != IndividualStyles) {
				char *allocation = AllocateAnnotation(pahSource->length, IndividualStyles);
				AnnotationHeader *pahAlloc = reinterpret_cast<AnnotationHeader *>(allocation);
				pahAlloc->length = pahSource->length;
				pahAlloc->lines = pahSource->lines;
				memcpy(allocation + sizeof(AnnotationHeader), annotations[line] + sizeof(AnnotationHeader), pahSource->length);
				delete []annotations[line];
				annotations[line] = allocation;
			}
		}
		AnnotationHeader *pah = reinterpret_cast<AnnotationHeader *>(annotations[line]);
		pah->style = IndividualStyles;
		memcpy(annotations[line] + sizeof(AnnotationHeader) + pah->length, styles, pah->length);
	}
}

int LineAnnotation::Length(int line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return reinterpret_cast<AnnotationHeader *>(annotations[line])->length;
	else
		return 0;
}

int LineAnnotation::Lines(int line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return reinterpret_cast<AnnotationHeader *>(annotations[line])->lines;
	else
		return 0;
}

LineTabstops::~LineTabstops() {
	Init();
}

void LineTabstops::Init() {
	for (int line = 0; line < tabstops.Length(); line++) {
		delete tabstops[line];
	}
	tabstops.DeleteAll();
}

void LineTabstops::InsertLine(int line) {
	if (tabstops.Length()) {
		tabstops.EnsureLength(line);
		tabstops.Insert(line, 0);
	}
}

void LineTabstops::RemoveLine(int line) {
	if (tabstops.Length() > line) {
		delete tabstops[line];
		tabstops.Delete(line);
	}
}

bool LineTabstops::ClearTabstops(int line) {
	if (line < tabstops.Length()) {
		TabstopList *tl = tabstops[line];
		if (tl) {
			tl->clear();
			return true;
		}
	}
	return false;
}

bool LineTabstops::AddTabstop(int line, int x) {
	tabstops.EnsureLength(line + 1);
	if (!tabstops[line]) {
		tabstops[line] = new TabstopList();
	}

	TabstopList *tl = tabstops[line];
	if (tl) {
		// tabstop positions are kept in order - insert in the right place
		std::vector<int>::iterator it = std::lower_bound(tl->begin(), tl->end(), x);
		// don't insert duplicates
		if (it == tl->end() || *it != x) {
			tl->insert(it, x);
			return true;
		}
	}
	return false;
}

int LineTabstops::GetNextTabstop(int line, int x) const {
	if (line < tabstops.Length()) {
		TabstopList *tl = tabstops[line];
		if (tl) {
			for (size_t i = 0; i < tl->size(); i++) {
				if ((*tl)[i] > x) {
					return (*tl)[i];
				}
			}
		}
	}
	return 0;
}
