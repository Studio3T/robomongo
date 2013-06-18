// Scintilla source code edit control
/** @file StyleContext.cxx
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
	unsigned int endPos;
	StyleContext &operator=(const StyleContext &);
	void GetNextChar(unsigned int pos) {
		chNext = static_cast<unsigned char>(styler.SafeGetCharAt(pos+1));
		if (styler.IsLeadByte(static_cast<char>(chNext))) {
			chNext = chNext << 8;
			chNext |= static_cast<unsigned char>(styler.SafeGetCharAt(pos+2));
		}
		// End of line?
		// Trigger on CR only (Mac style) or either on LF from CR+LF (Dos/Win)
		// or on LF alone (Unix). Avoid triggering two times on Dos/Win.
		atLineEnd = (ch == '\r' && chNext != '\n') ||
					(ch == '\n') ||
					(currentPos >= endPos);
	}

public:
	unsigned int currentPos;
	bool atLineStart;
	bool atLineEnd;
	int state;
	int chPrev;
	int ch;
	int chNext;

	StyleContext(unsigned int startPos, unsigned int length,
                        int initStyle, LexAccessor &styler_, char chMask=31) :
		styler(styler_),
		endPos(startPos + length),
		currentPos(startPos),
		atLineEnd(false),
		state(initStyle & chMask), // Mask off all bits which aren't in the chMask.
		chPrev(0),
		ch(0),
		chNext(0) {
		styler.StartAt(startPos, chMask);
		styler.StartSegment(startPos);
		atLineStart = static_cast<unsigned int>(styler.LineStart(styler.GetLine(startPos))) == startPos;
		unsigned int pos = currentPos;
		ch = static_cast<unsigned char>(styler.SafeGetCharAt(pos));
		if (styler.IsLeadByte(static_cast<char>(ch))) {
			pos++;
			ch = ch << 8;
			ch |= static_cast<unsigned char>(styler.SafeGetCharAt(pos));
		}
		GetNextChar(pos);
	}
	void Complete() {
		styler.ColourTo(currentPos - 1, state);
		styler.Flush();
	}
	bool More() const {
		return currentPos < endPos;
	}
	void Forward() {
		if (currentPos < endPos) {
			atLineStart = atLineEnd;
			chPrev = ch;
			currentPos++;
			if (ch >= 0x100)
				currentPos++;
			ch = chNext;
			GetNextChar(currentPos + ((ch >= 0x100) ? 1 : 0));
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
	void ChangeState(int state_) {
		state = state_;
	}
	void SetState(int state_) {
		styler.ColourTo(currentPos - 1, state);
		state = state_;
	}
	void ForwardSetState(int state_) {
		Forward();
		styler.ColourTo(currentPos - 1, state);
		state = state_;
	}
	int LengthCurrent() {
		return currentPos - styler.GetStartSegment();
	}
	int GetRelative(int n) {
		return static_cast<unsigned char>(styler.SafeGetCharAt(currentPos+n));
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
			if (*s != styler.SafeGetCharAt(currentPos+n))
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
				MakeLowerCase(static_cast<unsigned char>(styler.SafeGetCharAt(currentPos+n))))
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
