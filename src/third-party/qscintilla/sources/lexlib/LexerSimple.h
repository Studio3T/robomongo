// Scintilla source code edit control
/** @file LexerSimple.h
 ** A simple lexer with no state.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LEXERSIMPLE_H
#define LEXERSIMPLE_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

// A simple lexer with no state
class LexerSimple : public LexerBase {
	const LexerModule *module;
	std::string wordLists;
public:
	LexerSimple(const LexerModule *module_);
	const char * SCI_METHOD DescribeWordListSets();
	void SCI_METHOD Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
	void SCI_METHOD Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
