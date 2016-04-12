/** @file LexRust.cxx
 ** Lexer for Rust.
 **
 ** Copyright (c) 2013 by SiegeLord <slabode@aim.com>
 ** Converted to lexer object and added further folding features/properties by "Udo Lechner" <dlchnr(at)gmx(dot)net>
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <map>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "PropSetSimple.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static bool IsStreamCommentStyle(int style) {
	return style == SCE_RUST_COMMENTBLOCK ||
		   style == SCE_RUST_COMMENTBLOCKDOC;
}

// Options used for LexerRust
struct OptionsRust {
	bool fold;
	bool foldSyntaxBased;
	bool foldComment;
	bool foldCommentMultiline;
	bool foldCommentExplicit;
	std::string foldExplicitStart;
	std::string foldExplicitEnd;
	bool foldExplicitAnywhere;
	bool foldCompact;
	int  foldAtElseInt;
	bool foldAtElse;
	OptionsRust() {
		fold = false;
		foldSyntaxBased = true;
		foldComment = false;
		foldCommentMultiline = true;
		foldCommentExplicit = true;
		foldExplicitStart = "";
		foldExplicitEnd   = "";
		foldExplicitAnywhere = false;
		foldCompact = true;
		foldAtElseInt = -1;
		foldAtElse = false;
	}
};

static const char * const rustWordLists[] = {
			"Primary keywords and identifiers",
			"Built in types",
			"Other keywords",
			"Keywords 4",
			"Keywords 5",
			"Keywords 6",
			"Keywords 7",
			0,
		};

struct OptionSetRust : public OptionSet<OptionsRust> {
	OptionSetRust() {
		DefineProperty("fold", &OptionsRust::fold);

		DefineProperty("fold.comment", &OptionsRust::foldComment);

		DefineProperty("fold.compact", &OptionsRust::foldCompact);

		DefineProperty("fold.at.else", &OptionsRust::foldAtElse);

		DefineProperty("fold.rust.syntax.based", &OptionsRust::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("fold.rust.comment.multiline", &OptionsRust::foldCommentMultiline,
			"Set this property to 0 to disable folding multi-line comments when fold.comment=1.");

		DefineProperty("fold.rust.comment.explicit", &OptionsRust::foldCommentExplicit,
			"Set this property to 0 to disable folding explicit fold points when fold.comment=1.");

		DefineProperty("fold.rust.explicit.start", &OptionsRust::foldExplicitStart,
			"The string to use for explicit fold start points, replacing the standard //{.");

		DefineProperty("fold.rust.explicit.end", &OptionsRust::foldExplicitEnd,
			"The string to use for explicit fold end points, replacing the standard //}.");

		DefineProperty("fold.rust.explicit.anywhere", &OptionsRust::foldExplicitAnywhere,
			"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

		DefineProperty("lexer.rust.fold.at.else", &OptionsRust::foldAtElseInt,
			"This option enables Rust folding on a \"} else {\" line of an if statement.");

		DefineWordListSets(rustWordLists);
	}
};

class LexerRust : public ILexer {
	WordList keywords[7];
	OptionsRust options;
	OptionSetRust osRust;
public:
	virtual ~LexerRust() {
	}
	void SCI_METHOD Release() {
		delete this;
	}
	int SCI_METHOD Version() const {
		return lvOriginal;
	}
	const char * SCI_METHOD PropertyNames() {
		return osRust.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) {
		return osRust.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) {
		return osRust.DescribeProperty(name);
	}
	int SCI_METHOD PropertySet(const char *key, const char *val);
	const char * SCI_METHOD DescribeWordListSets() {
		return osRust.DescribeWordListSets();
	}
	int SCI_METHOD WordListSet(int n, const char *wl);
	void SCI_METHOD Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess);
	void * SCI_METHOD PrivateCall(int, void *) {
		return 0;
	}
	static ILexer *LexerFactoryRust() {
		return new LexerRust();
	}
};

int SCI_METHOD LexerRust::PropertySet(const char *key, const char *val) {
	if (osRust.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

int SCI_METHOD LexerRust::WordListSet(int n, const char *wl) {
	int firstModification = -1;
	if (n < 7) {
		WordList *wordListN = &keywords[n];
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0;
		}
	}
	return firstModification;
}

static bool IsWhitespace(int c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* This isn't quite right for Unicode identifiers */
static bool IsIdentifierStart(int ch) {
	return (IsASCII(ch) && (isalpha(ch) || ch == '_')) || !IsASCII(ch);
}

/* This isn't quite right for Unicode identifiers */
static bool IsIdentifierContinue(int ch) {
	return (IsASCII(ch) && (isalnum(ch) || ch == '_')) || !IsASCII(ch);
}

static void ScanWhitespace(Accessor& styler, int& pos, int max) {
	while (IsWhitespace(styler.SafeGetCharAt(pos, '\0')) && pos < max) {
		if (pos == styler.LineEnd(styler.GetLine(pos)))
			styler.SetLineState(styler.GetLine(pos), 0);
		pos++;
	}
	styler.ColourTo(pos-1, SCE_RUST_DEFAULT);
}

static void GrabString(char* s, Accessor& styler, int start, int len) {
	for (int ii = 0; ii < len; ii++)
		s[ii] = styler[ii + start];
	s[len] = '\0';
}

static void ScanIdentifier(Accessor& styler, int& pos, WordList *keywords) {
	int start = pos;
	while (IsIdentifierContinue(styler.SafeGetCharAt(pos, '\0')))
		pos++;

	if (styler.SafeGetCharAt(pos, '\0') == '!') {
		pos++;
		styler.ColourTo(pos - 1, SCE_RUST_MACRO);
	} else {
		char s[1024];
		int len = pos - start;
		len = len > 1024 ? 1024 : len;
		GrabString(s, styler, start, len);
		bool keyword = false;
		for (int ii = 0; ii < 7; ii++) {
			if (keywords[ii].InList(s)) {
				styler.ColourTo(pos - 1, SCE_RUST_WORD + ii);
				keyword = true;
				break;
			}
		}
		if (!keyword) {
			styler.ColourTo(pos - 1, SCE_RUST_IDENTIFIER);
		}
	}
}

static void ScanDigits(Accessor& styler, int& pos, int base) {
	for (;;) {
		int c = styler.SafeGetCharAt(pos, '\0');
		if (IsADigit(c, base) || c == '_')
			pos++;
		else
			break;
	}
}

static bool ScanExponent(Accessor& styler, int& pos) {
	int c = styler.SafeGetCharAt(pos, '\0');
	if (c == 'e' || c == 'E') {
		pos++;
		c = styler.SafeGetCharAt(pos, '\0');
		if (c == '-' || c == '+')
			pos++;
		int old_pos = pos;
		ScanDigits(styler, pos, 10);
		if (old_pos == pos)	{
			return false;
		}
	}
	return true;
}

static void ScanNumber(Accessor& styler, int& pos) {
	int base = 10;
	int c = styler.SafeGetCharAt(pos, '\0');
	int n = styler.SafeGetCharAt(pos + 1, '\0');
	bool error = false;
	if (c == '0' && n == 'x') {
        pos += 2;
        base = 16;
    } else if (c == '0' && n == 'b') {
        pos += 2;
        base = 2;
    }
    int old_pos = pos;
    ScanDigits(styler, pos, base);
    c = styler.SafeGetCharAt(pos, '\0');
    if (c == 'u' || c == 'i') {
		pos++;
		c = styler.SafeGetCharAt(pos, '\0');
		int n = styler.SafeGetCharAt(pos + 1, '\0');
		if (c == '8') {
			pos++;
		} else if (c == '1' && n == '6') {
			pos += 2;
		} else if (c == '3' && n == '2') {
			pos += 2;
		} else if (c == '6' && n == '4') {
			pos += 2;
		}
	} else if (c == '.') {
		error = base != 10;
		pos++;
		ScanDigits(styler, pos, 10);
		error |= !ScanExponent(styler, pos);
		c = styler.SafeGetCharAt(pos, '\0');
		if (c == 'f') {
			pos++;
			c = styler.SafeGetCharAt(pos, '\0');
			int n = styler.SafeGetCharAt(pos + 1, '\0');
			if (c == '3' && n == '2') {
				pos += 2;
			} else if (c == '6' && n == '4') {
				pos += 2;
			} else {
				error = true;
			}
		}
	}
	if (old_pos == pos) {
		error = true;
	}
	if (error)
		styler.ColourTo(pos - 1, SCE_RUST_LEXERROR);
	else
		styler.ColourTo(pos - 1, SCE_RUST_NUMBER);
}

static bool IsOneCharOperator(int c) {
	return c == ';' || c == ',' || c == '(' || c == ')'
	    || c == '{' || c == '}' || c == '[' || c == ']'
	    || c == '@' || c == '#' || c == '~' || c == '+'
	    || c == '*' || c == '/' || c == '^' || c == '%'
	    || c == '.' || c == ':' || c == '!' || c == '<'
	    || c == '>' || c == '=' || c == '-' || c == '&'
	    || c == '|' || c == '$';
}

static bool IsTwoCharOperator(int c, int n) {
	return (c == '.' && n == '.') || (c == ':' && n == ':')
	    || (c == '!' && n == '=') || (c == '<' && n == '<')
	    || (c == '<' && n == '=') || (c == '>' && n == '>')
	    || (c == '>' && n == '=') || (c == '=' && n == '=')
	    || (c == '=' && n == '>') || (c == '-' && n == '>')
	    || (c == '&' && n == '&') || (c == '|' && n == '|')
	    || (c == '-' && n == '=') || (c == '&' && n == '=')
	    || (c == '|' && n == '=') || (c == '+' && n == '=')
	    || (c == '*' && n == '=') || (c == '/' && n == '=')
	    || (c == '^' && n == '=') || (c == '%' && n == '=');
}

static bool IsThreeCharOperator(int c, int n, int n2) {
	return (c == '<' && n == '<' && n2 == '=')
	    || (c == '>' && n == '>' && n2 == '=');
}

static bool IsValidCharacterEscape(int c) {
	return c == 'n'  || c == 'r' || c == 't' || c == '\\'
	    || c == '\'' || c == '"' || c == '0';
}

static bool IsValidStringEscape(int c) {
	return IsValidCharacterEscape(c) || c == '\n';
}

static bool ScanNumericEscape(Accessor &styler, int& pos, int num_digits, bool stop_asap) {
	for (;;) {
		int c = styler.SafeGetCharAt(pos, '\0');
		if (!IsADigit(c, 16))
			break;
		num_digits--;
		pos++;
		if (num_digits == 0 && stop_asap)
			return true;
	}
	if (num_digits == 0) {
		return true;
	} else {
		return false;
	}
}

/* This is overly permissive for character literals in order to accept UTF-8 encoded
 * character literals. */
static void ScanCharacterLiteralOrLifetime(Accessor &styler, int& pos) {
	pos++;
	int c = styler.SafeGetCharAt(pos, '\0');
	int n = styler.SafeGetCharAt(pos + 1, '\0');
	bool done = false;
	bool valid_lifetime = IsIdentifierStart(c);
	bool valid_char = true;
	bool first = true;
	while (!done) {
		switch (c) {
			case '\\':
				done = true;
				if (IsValidCharacterEscape(n)) {
					pos += 2;
				} else if (n == 'x') {
					pos += 2;
					valid_char = ScanNumericEscape(styler, pos, 2, false);
				} else if (n == 'u') {
					pos += 2;
					valid_char = ScanNumericEscape(styler, pos, 4, false);
				} else if (n == 'U') {
					pos += 2;
					valid_char = ScanNumericEscape(styler, pos, 8, false);
				} else {
					valid_char = false;
				}
				break;
			case '\'':
				valid_char = !first;
				done = true;
				break;
			case '\t':
			case '\n':
			case '\r':
			case '\0':
				valid_char = false;
				done = true;
				break;
			default:
				if (!IsIdentifierContinue(c) && !first) {
					done = true;
				} else {
					pos++;
				}
				break;
		}
		c = styler.SafeGetCharAt(pos, '\0');
		n = styler.SafeGetCharAt(pos + 1, '\0');

		first = false;
	}
	if (styler.SafeGetCharAt(pos, '\0') == '\'') {
		valid_lifetime = false;
	} else {
		valid_char = false;
	}
	if (valid_lifetime) {
		styler.ColourTo(pos - 1, SCE_RUST_LIFETIME);
	} else if (valid_char) {
		pos++;
		styler.ColourTo(pos - 1, SCE_RUST_CHARACTER);
	} else {
		styler.ColourTo(pos - 1, SCE_RUST_LEXERROR);
	}
}

enum CommentState {
	UnknownComment,
	DocComment,
	NotDocComment
};

/*
 * The rule for block-doc comments is as follows (use x for asterisk)... /xx and /x! start doc comments
 * unless the entire comment is x's.
 */
static void ResumeBlockComment(Accessor &styler, int& pos, int max, CommentState state) {
	int c = styler.SafeGetCharAt(pos, '\0');
	bool maybe_doc_comment = false;
	bool any_non_asterisk = false;
	if (c == '*' || c == '!') {
		maybe_doc_comment = true;
	}
	for (;;) {
		if (pos == styler.LineEnd(styler.GetLine(pos)))
			styler.SetLineState(styler.GetLine(pos), 0);
		if (c == '*') {
			int n = styler.SafeGetCharAt(pos + 1, '\0');
			if (n == '/') {
				pos += 2;
				if (state == DocComment || (state == UnknownComment && maybe_doc_comment && any_non_asterisk))
					styler.ColourTo(pos - 1, SCE_RUST_COMMENTBLOCKDOC);
				else
					styler.ColourTo(pos - 1, SCE_RUST_COMMENTBLOCK);
				break;
			}
		} else {
			any_non_asterisk = true;
		}
		if (c == '\0' || pos >= max) {
			if (state == DocComment || (state == UnknownComment && maybe_doc_comment))
				styler.ColourTo(pos - 1, SCE_RUST_COMMENTBLOCKDOC);
			else
				styler.ColourTo(pos - 1, SCE_RUST_COMMENTBLOCK);
			break;
		}
		pos++;
		c = styler.SafeGetCharAt(pos, '\0');
	}
}

/*
 * The rule for line-doc comments is as follows... /// and //! start doc comments
 * unless the comment is composed entirely of /'s followed by whitespace. That is:
 * // - comment
 * /// - doc-comment
 * //// - comment
 * ////a - doc-comment
 */
static void ResumeLineComment(Accessor &styler, int& pos, int max, CommentState state) {
	bool maybe_doc_comment = false;
	int num_leading_slashes = 0;
	int c = styler.SafeGetCharAt(pos, '\0');
	if (c == '/') {
		num_leading_slashes = 1;
		while (pos < max) {
			pos++;
			c = styler.SafeGetCharAt(pos, '\0');
			if (c == '/') {
				num_leading_slashes++;
			} else {
				break;
			}
		}
	} else if (c == '!') {
		maybe_doc_comment = true;
	}

	bool non_white_space = false;
	while (pos < max && c != '\n' && c != '\0') {
		if (!IsWhitespace(c))
			non_white_space = true;
		if (pos == styler.LineEnd(styler.GetLine(pos)))
			styler.SetLineState(styler.GetLine(pos), 0);
		pos++;
		c = styler.SafeGetCharAt(pos, '\0');
	}

	maybe_doc_comment |= num_leading_slashes == 1 || (num_leading_slashes > 1 && non_white_space);

	if (state == DocComment || (state == UnknownComment && maybe_doc_comment))
		styler.ColourTo(pos - 1, SCE_RUST_COMMENTLINEDOC);
	else
		styler.ColourTo(pos - 1, SCE_RUST_COMMENTLINE);
}

static void ScanComments(Accessor &styler, int& pos, int max) {
	pos++;
	int c = styler.SafeGetCharAt(pos, '\0');
	pos++;
	if (c == '/')
		ResumeLineComment(styler, pos, max, UnknownComment);
	else if (c == '*')
		ResumeBlockComment(styler, pos, max, UnknownComment);
}

static void ResumeString(Accessor &styler, int& pos, int max) {
	int c = styler.SafeGetCharAt(pos, '\0');
	bool error = false;
	while (c != '"' && !error) {
		if (c == '\0' || pos >= max) {
			error = true;
			break;
		}
		if (pos == styler.LineEnd(styler.GetLine(pos)))
			styler.SetLineState(styler.GetLine(pos), 0);
		if (c == '\\') {
			int n = styler.SafeGetCharAt(pos + 1, '\0');
			if (IsValidStringEscape(n)) {
				pos += 2;
			} else if (n == 'x') {
				pos += 2;
				error = !ScanNumericEscape(styler, pos, 2, true);
			} else if (n == 'u') {
				pos += 2;
				error = !ScanNumericEscape(styler, pos, 4, true);
			} else if (n == 'U') {
				pos += 2;
				error = !ScanNumericEscape(styler, pos, 8, true);
			} else {
				pos += 1;
				error = true;
			}
		} else {
			pos++;
		}
		c = styler.SafeGetCharAt(pos, '\0');
	}
	if (!error)
		pos++;
	styler.ColourTo(pos - 1, SCE_RUST_STRING);
}

static void ResumeRawString(Accessor &styler, int& pos, int max, int num_hashes) {
	for (;;) {
		int c = styler.SafeGetCharAt(pos, '\0');
		if (c == '"') {
			pos++;
			int trailing_num_hashes = 0;
			while (styler.SafeGetCharAt(pos, '\0') == '#' && trailing_num_hashes < num_hashes) {
				trailing_num_hashes++;
				pos++;
			}
			if (trailing_num_hashes == num_hashes) {
				styler.SetLineState(styler.GetLine(pos), 0);
				styler.ColourTo(pos - 1, SCE_RUST_STRINGR);
				break;
			}
		} else if (c == '\0' || pos >= max) {
			styler.ColourTo(pos - 1, SCE_RUST_STRINGR);
			break;
		}
		if (pos == styler.LineEnd(styler.GetLine(pos)))
			styler.SetLineState(styler.GetLine(pos), num_hashes);
		pos++;
	}
}

static void ScanRawString(Accessor &styler, int& pos, int max) {
	pos++;
	int num_hashes = 0;
	while (styler.SafeGetCharAt(pos, '\0') == '#') {
		num_hashes++;
		pos++;
	}
	if (styler.SafeGetCharAt(pos, '\0') != '"') {
		styler.ColourTo(pos - 1, SCE_RUST_LEXERROR);
	} else {
		pos++;
		ResumeRawString(styler, pos, max, num_hashes);
	}
}

void SCI_METHOD LexerRust::Lex(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {
	PropSetSimple props;
	Accessor styler(pAccess, &props);
	int pos = startPos;
	int max = pos + length;

	styler.StartAt(pos);
	styler.StartSegment(pos);

	if (initStyle == SCE_RUST_COMMENTBLOCK || initStyle == SCE_RUST_COMMENTBLOCKDOC) {
		ResumeBlockComment(styler, pos, max, initStyle == SCE_RUST_COMMENTBLOCKDOC ? DocComment : NotDocComment);
	} else if (initStyle == SCE_RUST_COMMENTLINE || initStyle == SCE_RUST_COMMENTLINEDOC) {
		ResumeLineComment(styler, pos, max, initStyle == SCE_RUST_COMMENTLINEDOC ? DocComment : NotDocComment);
	} else if (initStyle == SCE_RUST_STRING) {
		ResumeString(styler, pos, max);
	} else if (initStyle == SCE_RUST_STRINGR) {
		ResumeRawString(styler, pos, max, styler.GetLineState(styler.GetLine(pos) - 1));
	}

	while (pos < max) {
		int c = styler.SafeGetCharAt(pos, '\0');
		int n = styler.SafeGetCharAt(pos + 1, '\0');
		int n2 = styler.SafeGetCharAt(pos + 2, '\0');

		if (pos == 0 && c == '#' && n == '!') {
			pos += 2;
			ResumeLineComment(styler, pos, max, NotDocComment);
		} else if (IsWhitespace(c)) {
			ScanWhitespace(styler, pos, max);
		} else if (c == '/' && (n == '/' || n == '*')) {
			ScanComments(styler, pos, max);
		} else if (c == 'r' && (n == '#' || n == '"')) {
			ScanRawString(styler, pos, max);
		} else if (IsIdentifierStart(c)) {
			ScanIdentifier(styler, pos, keywords);
		} else if (IsADigit(c)) {
			ScanNumber(styler, pos);
		} else if (IsThreeCharOperator(c, n, n2)) {
			pos += 3;
			styler.ColourTo(pos - 1, SCE_RUST_OPERATOR);
		} else if (IsTwoCharOperator(c, n)) {
			pos += 2;
			styler.ColourTo(pos - 1, SCE_RUST_OPERATOR);
		} else if (IsOneCharOperator(c)) {
			pos++;
			styler.ColourTo(pos - 1, SCE_RUST_OPERATOR);
		} else if (c == '\'') {
			ScanCharacterLiteralOrLifetime(styler, pos);
		} else if (c == '"') {
			pos++;
			ResumeString(styler, pos, max);
		} else {
			pos++;
			styler.ColourTo(pos - 1, SCE_RUST_LEXERROR);
		}
	}
	styler.ColourTo(pos - 1, SCE_RUST_DEFAULT);
	styler.Flush();
}

void SCI_METHOD LexerRust::Fold(unsigned int startPos, int length, int initStyle, IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	unsigned int endPos = startPos + length;
	int visibleChars = 0;
	bool inLineComment = false;
	int lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	unsigned int lineStartNext = styler.LineStart(lineCurrent+1);
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (unsigned int i = startPos; i < endPos; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		bool atEOL = i == (lineStartNext-1);
		if ((style == SCE_RUST_COMMENTLINE) || (style == SCE_RUST_COMMENTLINEDOC))
			inLineComment = true;
		if (options.foldComment && options.foldCommentMultiline && IsStreamCommentStyle(style) && !inLineComment) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldComment && options.foldCommentExplicit && ((style == SCE_RUST_COMMENTLINE) || options.foldExplicitAnywhere)) {
			if (userDefinedFoldMarkers) {
				if (styler.Match(i, options.foldExplicitStart.c_str())) {
					levelNext++;
				} else if (styler.Match(i, options.foldExplicitEnd.c_str())) {
					levelNext--;
				}
			} else {
				if ((ch == '/') && (chNext == '/')) {
					char chNext2 = styler.SafeGetCharAt(i + 2);
					if (chNext2 == '{') {
						levelNext++;
					} else if (chNext2 == '}') {
						levelNext--;
					}
				}
			}
		}
		if (options.foldSyntaxBased && (style == SCE_RUST_OPERATOR)) {
			if (ch == '{') {
				// Measure the minimum before a '{' to allow
				// folding on "} else {"
				if (levelMinCurrent > levelNext) {
					levelMinCurrent = levelNext;
				}
				levelNext++;
			} else if (ch == '}') {
				levelNext--;
			}
		}
		if (!IsASpace(ch))
			visibleChars++;
		if (atEOL || (i == endPos-1)) {
			int levelUse = levelCurrent;
			if (options.foldSyntaxBased && options.foldAtElse) {
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			lineStartNext = styler.LineStart(lineCurrent+1);
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			if (atEOL && (i == static_cast<unsigned int>(styler.Length()-1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
			inLineComment = false;
		}
	}
}

LexerModule lmRust(SCLEX_RUST, LexerRust::LexerFactoryRust, "rust", rustWordLists);
