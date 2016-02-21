// Scintilla source code edit control
/** @file StyleContext.h
 ** Lexer infrastructure.
 **/
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#ifndef STYLECONTEXT_H
#define STYLECONTEXT_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

static inline int MakeLowerCase(int ch) {
	if (ch < 'A' || ch > 'Z')
		return ch;
	else
		return ch - 'A' + 'a';
}

// All languages handled so far can treat all characters >= 0x80 as one class
// which just continues the current token or starts an identifier if in default.
// DBCS treated specially as the second character can be < 0x80 and hence
// syntactically significant. UTF-8 avoids this as all trail bytes are >= 0x80
class StyleContext {
	LexAccessor &styler;
	IDocumentWithLineEnd *multiByteAccess;
	unsigned int endPos;
	unsigned int lengthDocument;

	// Used for optimizing GetRelativeCharacter
	unsigned int posRelative;
	unsigned int currentPosLastRelative;
	int offsetRelative;

	StyleContext &operator=(const StyleContext &);

	void GetNextChar() {
		if (multiByteAccess) {
			chNext = multiByteAccess->GetCharacterAndWidth(currentPos+width, &widthNext);
		} else {
			chNext = static_cast<unsigned char>(styler.SafeGetCharAt(currentPos+width, 0));
			widthNext = 1;
		}
		// End of line determined from line end position, allowing CR, LF,
		// CRLF and Unicode line ends as set by document.
		if (currentLine < lineDocEnd)
			atLineEnd = static_cast<int>(currentPos) >= (lineStartNext-1);
		else // Last line
			atLineEnd = static_cast<int>(currentPos) >= lineStartNext;
	}

public:
	unsigned int currentPos;
	int currentLine;
	int lineDocEnd;
	int lineStartNext;
	bool atLineStart;
	bool atLineEnd;
	int state;
	int chPrev;
	int ch;
	int width;
	int chNext;
	int widthNext;

	StyleContext(unsigned int startPos, unsigned int length,
                        int initStyle, LexAccessor &styler_, char chMask='\377') :
		styler(styler_),
		multiByteAccess(0),
		endPos(startPos + length),
		posRelative(0),
		currentPosLastRelative(0x7FFFFFFF),
		offsetRelative(0),
		currentPos(startPos),
		currentLine(-1),
		lineStartNext(-1),
		atLineEnd(false),
		state(initStyle & chMask), // Mask off all bits which aren't in the chMask.
		chPrev(0),
		ch(0),
		width(0),
		chNext(0),
		widthNext(1) {
		if (styler.Encoding() != enc8bit) {
			multiByteAccess = styler.MultiByteAccess();
		}
		styler.StartAt(startPos /*, chMask*/);
		styler.StartSegment(startPos);
		currentLine = styler.GetLine(startPos);
		lineStartNext = styler.LineStart(currentLine+1);
		lengthDocument = static_cast<unsigned int>(styler.Length());
		if (endPos == lengthDocument)
			endPos++;
		lineDocEnd = styler.GetLine(lengthDocument);
		atLineStart = static_cast<unsigned int>(styler.LineStart(currentLine)) == startPos;

		// Variable width is now 0 so GetNextChar gets the char at currentPos into chNext/widthNext
		width = 0;
		GetNextChar();
		ch = chNext;
		width = widthNext;

		GetNextChar();
	}
	void Complete() {
		styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
		styler.Flush();
	}
	bool More() const {
		return currentPos < endPos;
	}
	void Forward() {
		if (currentPos < endPos) {
			atLineStart = atLineEnd;
			if (atLineStart) {
				currentLine++;
				lineStartNext = styler.LineStart(currentLine+1);
			}
			chPrev = ch;
			currentPos += width;
			ch = chNext;
			width = widthNext;
			GetNextChar();
		} else {
			atLineStart = false;
			chPrev = ' ';
			ch = ' ';
			chNext = ' ';
			atLineEnd = true;
		}
	}
	void Forward(int nb) {
		for (int i = 0; i < nb; i++) {
			Forward();
		}
	}
	void ForwardBytes(int nb) {
		size_t forwardPos = currentPos + nb;
		while (forwardPos > currentPos) {
			Forward();
		}
	}
	void ChangeState(int state_) {
		state = state_;
	}
	void SetState(int state_) {
		styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
		state = state_;
	}
	void ForwardSetState(int state_) {
		Forward();
		styler.ColourTo(currentPos - ((currentPos > lengthDocument) ? 2 : 1), state);
		state = state_;
	}
	int LengthCurrent() const {
		return currentPos - styler.GetStartSegment();
	}
	int GetRelative(int n) {
		return static_cast<unsigned char>(styler.SafeGetCharAt(currentPos+n, 0));
	}
	int GetRelativeCharacter(int n) {
		if (n == 0)
			return ch;
		if (multiByteAccess) {
			if ((currentPosLastRelative != currentPos) ||
				((n > 0) && ((offsetRelative < 0) || (n < offsetRelative))) ||
				((n < 0) && ((offsetRelative > 0) || (n > offsetRelative)))) {
				posRelative = currentPos;
				offsetRelative = 0;
			}
			int diffRelative = n - offsetRelative;
			int posNew = multiByteAccess->GetRelativePosition(posRelative, diffRelative);
			int chReturn = multiByteAccess->GetCharacterAndWidth(posNew, 0);
			posRelative = posNew;
			currentPosLastRelative = currentPos;
			offsetRelative = n;
			return chReturn;
		} else {
			// fast version for single byte encodings
			return static_cast<unsigned char>(styler.SafeGetCharAt(currentPos + n, 0));
		}
	}
	bool Match(char ch0) const {
		return ch == static_cast<unsigned char>(ch0);
	}
	bool Match(char ch0, char ch1) const {
		return (ch == static_cast<unsigned char>(ch0)) && (chNext == static_cast<unsigned char>(ch1));
	}
	bool Match(const char *s) {
		if (ch != static_cast<unsigned char>(*s))
			return false;
		s++;
		if (!*s)
			return true;
		if (chNext != static_cast<unsigned char>(*s))
			return false;
		s++;
		for (int n=2; *s; n++) {
			if (*s != styler.SafeGetCharAt(currentPos+n, 0))
				return false;
			s++;
		}
		return true;
	}
	bool MatchIgnoreCase(const char *s) {
		if (MakeLowerCase(ch) != static_cast<unsigned char>(*s))
			return false;
		s++;
		if (MakeLowerCase(chNext) != static_cast<unsigned char>(*s))
			return false;
		s++;
		for (int n=2; *s; n++) {
			if (static_cast<unsigned char>(*s) !=
				MakeLowerCase(static_cast<unsigned char>(styler.SafeGetCharAt(currentPos+n, 0))))
				return false;
			s++;
		}
		return true;
	}
	// Non-inline
	void GetCurrent(char *s, unsigned int len);
	void GetCurrentLowered(char *s, unsigned int len);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
