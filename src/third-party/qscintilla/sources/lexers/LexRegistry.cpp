// Scintilla source code edit control
/**
 * @file LexRegistry.cxx
 * @date July 26 2014
 * @brief Lexer for Windows registration files(.reg)
 * @author nkmathew
 *
 * The License.txt file describes the conditions under which this software may be
 * distributed.
 *
 */

#include <cstdlib>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include <map>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static const char *const RegistryWordListDesc[] = {
	0
};

struct OptionsRegistry {
	bool foldCompact;
	bool fold;
	OptionsRegistry() {
		foldCompact = false;
		fold = false;
	}
};

struct OptionSetRegistry : public OptionSet<OptionsRegistry> {
	OptionSetRegistry() {
		DefineProperty("fold.compact", &OptionsRegistry::foldCompact);
		DefineProperty("fold", &OptionsRegistry::fold);
		DefineWordListSets(RegistryWordListDesc);
	}
};

class LexerRegistry : public ILexer {
	OptionsRegistry options;
	OptionSetRegistry optSetRegistry;

	static bool IsStringState(int state) {
		return (state == SCE_REG_VALUENAME || state == SCE_REG_STRING);
	}

	static bool IsKeyPathState(int state) {
		return (state == SCE_REG_ADDEDKEY || state == SCE_REG_DELETEDKEY);
	}

	static bool AtValueType(LexAccessor &styler, int start) {
		int i = 0;
		while (i < 10) {
			i++;
			char curr = styler.SafeGetCharAt(start+i, '\0');
			if (curr == ':') {
				return true;
			} else if (!curr) {
				return false;
			}
		}
		return false;
	}

	static bool IsNextNonWhitespace(LexAccessor &styler, int start, char ch) {
		int i = 0;
		while (i < 100) {
			i++;
			char curr = styler.SafeGetCharAt(start+i, '\0');
			char next = styler.SafeGetCharAt(start+i+1, '\0');
			bool atEOL = (curr == '\r' && next != '\n') || (curr == '\n');
			if (curr == ch) {
				return true;
			} else if (!isspacechar(curr) || atEOL) {
				return false;
			}
		}
		return false;
	}

	// Looks for the equal sign at the end of the string
	static bool AtValueName(LexAccessor &styler, int start) {
		bool atEOL = false;
		int i = 0;
		bool escaped = false;
		while (!atEOL) {
			i++;
			char curr = styler.SafeGetCharAt(start+i, '\0');
			char next = styler.SafeGetCharAt(start+i+1, '\0');
			atEOL = (curr == '\r' && next != '\n') || (curr == '\n');
			if (escaped) {
				escaped = false;
				continue;
			}
			escaped = curr == '\\';
			if (curr == '"') {
				return IsNextNonWhitespace(styler, start+i, '=');
			} else if (!curr) {
				return false;
			}
		}
		return false;
	}

	static bool AtKeyPathEnd(LexAccessor &styler, int start) {
		bool atEOL = false;
		int i = 0;
		while (!atEOL) {
			i++;
			char curr = styler.SafeGetCharAt(start+i, '\0');
			char next = styler.SafeGetCharAt(start+i+1, '\0');
			atEOL = (curr == '\r' && next != '\n') || (curr == '\n');
			if (curr == ']' || !curr) {
				// There's still at least one or more square brackets ahead
				return false;
			}
		}
		return true;
	}

	static bool AtGUID(LexAccessor &styler, int start) {
		int count = 8;
		int portion = 0;
		int offset = 1;
		char digit = '\0';
		while (portion < 5) {
			int i = 0;
			while (i < count) {
				digit = styler.SafeGetCharAt(start+offset);
				if (!(isxdigit(digit) || digit == '-')) {
					return false;
				}
				offset++;
				i++;
			}
			portion++;
			count = (portion == 4) ? 13 : 5;
		}
		digit = styler.SafeGetCharAt(start+offset);
		if (digit == '}') {
			return true;
		} else {
			return false;
		}
	}

public:
	LexerRegistry() {}
	virtual ~LexerRegistry() {}
	virtual int SCI_METHOD Version() const {
		return lvOriginal;
	}
	virtual void SCI_METHOD Release() {
		delete this;
	}
	virtual const char *SCI_METHOD PropertyNames() {
		return optSetRegistry.PropertyNames();
	}
	virtual int SCI_METHOD PropertyType(const char *name) {
		return optSetRegistry.PropertyType(name);
	}
	virtual const char *SCI_METHOD DescribeProperty(const char *name) {
		return optSetRegistry.DescribeProperty(name);
	}
	virtual int SCI_METHOD PropertySet(const char *key, const char *val) {
		if (optSetRegistry.PropertySet(&options, key, val)) {
			return 0;
		}
		return -1;
	}
	virtual int SCI_METHOD WordListSet(int, const char *) {
		return -1;
	}
	virtual void *SCI_METHOD PrivateCall(int, void *) {
		return 0;
	}
	static ILexer *LexerFactoryRegistry() {
		return new LexerRegistry;
	}
	virtual const char *SCI_METHOD DescribeWordListSets() {
		return optSetRegistry.DescribeWordListSets();
	}
	virtual void SCI_METHOD Lex(unsigned startPos,
								int length,
								int initStyle,
								IDocument *pAccess);
	virtual void SCI_METHOD Fold(unsigned startPos,
								 int length,
								 int initStyle,
								 IDocument *pAccess);
};

void SCI_METHOD LexerRegistry::Lex(unsigned startPos,
								   int length,
								   int initStyle,
								   IDocument *pAccess) {
	int beforeGUID = SCE_REG_DEFAULT;
	int beforeEscape = SCE_REG_DEFAULT;
	CharacterSet setOperators = CharacterSet(CharacterSet::setNone, "-,.=:\\@()");
	LexAccessor styler(pAccess);
	StyleContext context(startPos, length, initStyle, styler);
	bool highlight = true;
	bool afterEqualSign = false;
	while (context.More()) {
		if (context.atLineStart) {
			int currPos = static_cast<int>(context.currentPos);
			bool continued = styler[currPos-3] == '\\';
			highlight = continued ? true : false;
		}
		switch (context.state) {
			case SCE_REG_COMMENT:
				if (context.atLineEnd) {
					context.SetState(SCE_REG_DEFAULT);
				}
				break;
			case SCE_REG_VALUENAME:
			case SCE_REG_STRING: {
					int currPos = static_cast<int>(context.currentPos);
					if (context.ch == '"') {
						context.ForwardSetState(SCE_REG_DEFAULT);
					} else if (context.ch == '\\') {
						beforeEscape = context.state;
						context.SetState(SCE_REG_ESCAPED);
						context.Forward();
					} else if (context.ch == '{') {
						if (AtGUID(styler, currPos)) {
							beforeGUID = context.state;
							context.SetState(SCE_REG_STRING_GUID);
						}
					}
					if (context.state == SCE_REG_STRING &&
						context.ch == '%' &&
						(isdigit(context.chNext) || context.chNext == '*')) {
						context.SetState(SCE_REG_PARAMETER);
					}
				}
				break;
			case SCE_REG_PARAMETER:
				context.ForwardSetState(SCE_REG_STRING);
				if (context.ch == '"') {
					context.ForwardSetState(SCE_REG_DEFAULT);
				}
				break;
			case SCE_REG_VALUETYPE:
				if (context.ch == ':') {
					context.SetState(SCE_REG_DEFAULT);
					afterEqualSign = false;
				}
				break;
			case SCE_REG_HEXDIGIT:
			case SCE_REG_OPERATOR:
				context.SetState(SCE_REG_DEFAULT);
				break;
			case SCE_REG_DELETEDKEY:
			case SCE_REG_ADDEDKEY: {
					int currPos = static_cast<int>(context.currentPos);
					if (context.ch == ']' && AtKeyPathEnd(styler, currPos)) {
						context.ForwardSetState(SCE_REG_DEFAULT);
					} else if (context.ch == '{') {
						if (AtGUID(styler, currPos)) {
							beforeGUID = context.state;
							context.SetState(SCE_REG_KEYPATH_GUID);
						}
					}
				}
				break;
			case SCE_REG_ESCAPED:
				if (context.ch == '"') {
					context.SetState(beforeEscape);
					context.ForwardSetState(SCE_REG_DEFAULT);
				} else if (context.ch == '\\') {
					context.Forward();
				} else {
					context.SetState(beforeEscape);
					beforeEscape = SCE_REG_DEFAULT;
				}
				break;
			case SCE_REG_STRING_GUID:
			case SCE_REG_KEYPATH_GUID: {
					if (context.ch == '}') {
						context.ForwardSetState(beforeGUID);
						beforeGUID = SCE_REG_DEFAULT;
					}
					int currPos = static_cast<int>(context.currentPos);
					if (context.ch == '"' && IsStringState(context.state)) {
						context.ForwardSetState(SCE_REG_DEFAULT);
					} else if (context.ch == ']' &&
							   AtKeyPathEnd(styler, currPos) &&
							   IsKeyPathState(context.state)) {
						context.ForwardSetState(SCE_REG_DEFAULT);
					} else if (context.ch == '\\' && IsStringState(context.state)) {
						beforeEscape = context.state;
						context.SetState(SCE_REG_ESCAPED);
						context.Forward();
					}
				}
				break;
		}
		// Determine if a new state should be entered.
		if (context.state == SCE_REG_DEFAULT) {
			int currPos = static_cast<int>(context.currentPos);
			if (context.ch == ';') {
				context.SetState(SCE_REG_COMMENT);
			} else if (context.ch == '"') {
				if (AtValueName(styler, currPos)) {
					context.SetState(SCE_REG_VALUENAME);
				} else {
					context.SetState(SCE_REG_STRING);
				}
			} else if (context.ch == '[') {
				if (IsNextNonWhitespace(styler, currPos, '-')) {
					context.SetState(SCE_REG_DELETEDKEY);
				} else {
					context.SetState(SCE_REG_ADDEDKEY);
				}
			} else if (context.ch == '=') {
				afterEqualSign = true;
				highlight = true;
			} else if (afterEqualSign) {
				bool wordStart = isalpha(context.ch) && !isalpha(context.chPrev);
				if (wordStart && AtValueType(styler, currPos)) {
					context.SetState(SCE_REG_VALUETYPE);
				}
			} else if (isxdigit(context.ch) && highlight) {
				context.SetState(SCE_REG_HEXDIGIT);
			}
			highlight = (context.ch == '@') ? true : highlight;
			if (setOperators.Contains(context.ch) && highlight) {
				context.SetState(SCE_REG_OPERATOR);
			}
		}
		context.Forward();
	}
	context.Complete();
}

// Folding similar to that of FoldPropsDoc in LexOthers
void SCI_METHOD LexerRegistry::Fold(unsigned startPos,
									int length,
									int,
									IDocument *pAccess) {
	if (!options.fold) {
		return;
	}
	LexAccessor styler(pAccess);
	int currLine = styler.GetLine(startPos);
	int visibleChars = 0;
	unsigned endPos = startPos + length;
	bool atKeyPath = false;
	for (unsigned i = startPos; i < endPos; i++) {
		atKeyPath = IsKeyPathState(styler.StyleAt(i)) ? true : atKeyPath;
		char curr = styler.SafeGetCharAt(i);
		char next = styler.SafeGetCharAt(i+1);
		bool atEOL = (curr == '\r' && next != '\n') || (curr == '\n');
		if (atEOL || i == (endPos-1)) {
			int level = SC_FOLDLEVELBASE;
			if (currLine > 0) {
				int prevLevel = styler.LevelAt(currLine-1);
				if (prevLevel & SC_FOLDLEVELHEADERFLAG) {
					level += 1;
				} else {
					level = prevLevel;
				}
			}
			if (!visibleChars && options.foldCompact) {
				level |= SC_FOLDLEVELWHITEFLAG;
			} else if (atKeyPath) {
				level = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
			}
			if (level != styler.LevelAt(currLine)) {
				styler.SetLevel(currLine, level);
			}
			currLine++;
			visibleChars = 0;
			atKeyPath = false;
		}
		if (!isspacechar(curr)) {
			visibleChars++;
		}
	}

	// Make the folding reach the last line in the file
	int level = SC_FOLDLEVELBASE;
	if (currLine > 0) {
		int prevLevel = styler.LevelAt(currLine-1);
		if (prevLevel & SC_FOLDLEVELHEADERFLAG) {
			level += 1;
		} else {
			level = prevLevel;
		}
	}
	styler.SetLevel(currLine, level);
}

LexerModule lmRegistry(SCLEX_REGISTRY,
					   LexerRegistry::LexerFactoryRegistry,
					   "registry",
					   RegistryWordListDesc);

