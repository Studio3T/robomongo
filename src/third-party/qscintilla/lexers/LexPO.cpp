// Scintilla source code edit control
/** @file LexPO.cxx
 ** Lexer for GetText Translation (PO) files.
 **/
// Copyright 2012 by Colomban Wendling <ban@herbesfolles.org>
// The License.txt file describes the conditions under which this software may be distributed.

// see https://www.gnu.org/software/gettext/manual/gettext.html#PO-Files for the syntax reference
// some details are taken from the GNU msgfmt behavior (like that indent is allows in front of lines)

// TODO:
// * add keywords for flags (fuzzy, c-format, ...)
// * highlight formats inside c-format strings (%s, %d, etc.)
// * style for previous untranslated string? ("#|" comment)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static void ColourisePODoc(unsigned int startPos, int length, int initStyle, WordList *[], Accessor &styler) {
	StyleContext sc(startPos, length, initStyle, styler);
	bool escaped = false;
	int curLine = styler.GetLine(startPos);
	// the line state holds the last state on or before the line that isn't the default style
	int curLineState = curLine > 0 ? styler.GetLineState(curLine - 1) : SCE_PO_DEFAULT;
	
	for (; sc.More(); sc.Forward()) {
		// whether we should leave a state
		switch (sc.state) {
			case SCE_PO_COMMENT:
			case SCE_PO_PROGRAMMER_COMMENT:
			case SCE_PO_REFERENCE:
			case SCE_PO_FLAGS:
			case SCE_PO_FUZZY:
				if (sc.atLineEnd)
					sc.SetState(SCE_PO_DEFAULT);
				else if (sc.state == SCE_PO_FLAGS && sc.Match("fuzzy"))
					// here we behave like the previous parser, but this should probably be highlighted
					// on its own like a keyword rather than changing the whole flags style
					sc.ChangeState(SCE_PO_FUZZY);
				break;
			
			case SCE_PO_MSGCTXT:
			case SCE_PO_MSGID:
			case SCE_PO_MSGSTR:
				if (isspacechar(sc.ch))
					sc.SetState(SCE_PO_DEFAULT);
				break;
			
			case SCE_PO_ERROR:
				if (sc.atLineEnd)
					sc.SetState(SCE_PO_DEFAULT);
				break;
			
			case SCE_PO_MSGCTXT_TEXT:
			case SCE_PO_MSGID_TEXT:
			case SCE_PO_MSGSTR_TEXT:
				if (sc.atLineEnd) { // invalid inside a string
					if (sc.state == SCE_PO_MSGCTXT_TEXT)
						sc.ChangeState(SCE_PO_MSGCTXT_TEXT_EOL);
					else if (sc.state == SCE_PO_MSGID_TEXT)
						sc.ChangeState(SCE_PO_MSGID_TEXT_EOL);
					else if (sc.state == SCE_PO_MSGSTR_TEXT)
						sc.ChangeState(SCE_PO_MSGSTR_TEXT_EOL);
					sc.SetState(SCE_PO_DEFAULT);
					escaped = false;
				} else {
					if (escaped)
						escaped = false;
					else if (sc.ch == '\\')
						escaped = true;
					else if (sc.ch == '"')
						sc.ForwardSetState(SCE_PO_DEFAULT);
				}
				break;
		}
		
		// whether we should enter a new state
		if (sc.state == SCE_PO_DEFAULT) {
			// forward to the first non-white character on the line
			bool atLineStart = sc.atLineStart;
			if (atLineStart) {
				while (sc.More() && ! sc.atLineEnd && isspacechar(sc.ch))
					sc.Forward();
			}
			
			if (atLineStart && sc.ch == '#') {
				if (sc.chNext == '.')
					sc.SetState(SCE_PO_PROGRAMMER_COMMENT);
				else if (sc.chNext == ':')
					sc.SetState(SCE_PO_REFERENCE);
				else if (sc.chNext == ',')
					sc.SetState(SCE_PO_FLAGS);
				else
					sc.SetState(SCE_PO_COMMENT);
			} else if (atLineStart && sc.Match("msgid")) { // includes msgid_plural
				sc.SetState(SCE_PO_MSGID);
			} else if (atLineStart && sc.Match("msgstr")) { // includes [] suffixes
				sc.SetState(SCE_PO_MSGSTR);
			} else if (atLineStart && sc.Match("msgctxt")) {
				sc.SetState(SCE_PO_MSGCTXT);
			} else if (sc.ch == '"') {
				if (curLineState == SCE_PO_MSGCTXT || curLineState == SCE_PO_MSGCTXT_TEXT)
					sc.SetState(SCE_PO_MSGCTXT_TEXT);
				else if (curLineState == SCE_PO_MSGID || curLineState == SCE_PO_MSGID_TEXT)
					sc.SetState(SCE_PO_MSGID_TEXT);
				else if (curLineState == SCE_PO_MSGSTR || curLineState == SCE_PO_MSGSTR_TEXT)
					sc.SetState(SCE_PO_MSGSTR_TEXT);
				else
					sc.SetState(SCE_PO_ERROR);
			} else if (! isspacechar(sc.ch))
				sc.SetState(SCE_PO_ERROR);
			
			if (sc.state != SCE_PO_DEFAULT)
				curLineState = sc.state;
		}
		
		if (sc.atLineEnd) {
			// Update the line state, so it can be seen by next line
			curLine = styler.GetLine(sc.currentPos);
			styler.SetLineState(curLine, curLineState);
		}
	}
	sc.Complete();
}

static const char *const poWordListDesc[] = {
	0
};

LexerModule lmPO(SCLEX_PO, ColourisePODoc, "po", 0, poWordListDesc);
