// Scintilla source code edit control
/** @file AutoComplete.cxx
 ** Defines the auto completion list box.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <string>

#include "Platform.h"

#include "CharacterSet.h"
#include "AutoComplete.h"
#include "Scintilla.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

AutoComplete::AutoComplete() :
	active(false),
	separator(' '),
	typesep('?'),
	ignoreCase(false),
	chooseSingle(false),
	lb(0),
	posStart(0),
	startLen(0),
	cancelAtStartPos(true),
	autoHide(true),
	dropRestOfWord(false),
	ignoreCaseBehaviour(SC_CASEINSENSITIVEBEHAVIOUR_RESPECTCASE),
	widthLBDefault(100),
	heightLBDefault(100) {
	lb = ListBox::Allocate();
	stopChars[0] = '\0';
	fillUpChars[0] = '\0';
}

AutoComplete::~AutoComplete() {
	if (lb) {
		lb->Destroy();
		delete lb;
		lb = 0;
	}
}

bool AutoComplete::Active() const {
	return active;
}

void AutoComplete::Start(Window &parent, int ctrlID,
	int position, Point location, int startLen_,
	int lineHeight, bool unicodeMode, int technology) {
	if (active) {
		Cancel();
	}
	lb->Create(parent, ctrlID, location, lineHeight, unicodeMode, technology);
	lb->Clear();
	active = true;
	startLen = startLen_;
	posStart = position;
}

void AutoComplete::SetStopChars(const char *stopChars_) {
	strncpy(stopChars, stopChars_, sizeof(stopChars));
	stopChars[sizeof(stopChars) - 1] = '\0';
}

bool AutoComplete::IsStopChar(char ch) {
	return ch && strchr(stopChars, ch);
}

void AutoComplete::SetFillUpChars(const char *fillUpChars_) {
	strncpy(fillUpChars, fillUpChars_, sizeof(fillUpChars));
	fillUpChars[sizeof(fillUpChars) - 1] = '\0';
}

bool AutoComplete::IsFillUpChar(char ch) {
	return ch && strchr(fillUpChars, ch);
}

void AutoComplete::SetSeparator(char separator_) {
	separator = separator_;
}

char AutoComplete::GetSeparator() const {
	return separator;
}

void AutoComplete::SetTypesep(char separator_) {
	typesep = separator_;
}

char AutoComplete::GetTypesep() const {
	return typesep;
}

void AutoComplete::SetList(const char *list) {
	lb->SetList(list, separator, typesep);
}

int AutoComplete::GetSelection() const {
	return lb->GetSelection();
}

std::string AutoComplete::GetValue(int item) const {
	char value[maxItemLen];
	lb->GetValue(item, value, sizeof(value));
	return std::string(value);
}

void AutoComplete::Show(bool show) {
	lb->Show(show);
	if (show)
		lb->Select(0);
}

void AutoComplete::Cancel() {
	if (lb->Created()) {
		lb->Clear();
		lb->Destroy();
		active = false;
	}
}


void AutoComplete::Move(int delta) {
	int count = lb->Length();
	int current = lb->GetSelection();
	current += delta;
	if (current >= count)
		current = count - 1;
	if (current < 0)
		current = 0;
	lb->Select(current);
}

void AutoComplete::Select(const char *word) {
	size_t lenWord = strlen(word);
	int location = -1;
	int start = 0; // lower bound of the api array block to search
	int end = lb->Length() - 1; // upper bound of the api array block to search
	while ((start <= end) && (location == -1)) { // Binary searching loop
		int pivot = (start + end) / 2;
		char item[maxItemLen];
		lb->GetValue(pivot, item, maxItemLen);
		int cond;
		if (ignoreCase)
			cond = CompareNCaseInsensitive(word, item, lenWord);
		else
			cond = strncmp(word, item, lenWord);
		if (!cond) {
			// Find first match
			while (pivot > start) {
				lb->GetValue(pivot-1, item, maxItemLen);
				if (ignoreCase)
					cond = CompareNCaseInsensitive(word, item, lenWord);
				else
					cond = strncmp(word, item, lenWord);
				if (0 != cond)
					break;
				--pivot;
			}
			location = pivot;
			if (ignoreCase
				&& ignoreCaseBehaviour == SC_CASEINSENSITIVEBEHAVIOUR_RESPECTCASE) {
				// Check for exact-case match
				for (; pivot <= end; pivot++) {
					lb->GetValue(pivot, item, maxItemLen);
					if (!strncmp(word, item, lenWord)) {
						location = pivot;
						break;
					}
					if (CompareNCaseInsensitive(word, item, lenWord))
						break;
				}
			}
		} else if (cond < 0) {
			end = pivot - 1;
		} else if (cond > 0) {
			start = pivot + 1;
		}
	}
	if (location == -1 && autoHide)
		Cancel();
	else
		lb->Select(location);
}

