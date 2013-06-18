// Scintilla source code edit control
/** @file ViewStyle.cxx
 ** Store information on how the document is to be viewed.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>

#include <vector>
#include <map>

#include "Platform.h"

#include "Scintilla.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

MarginStyle::MarginStyle() :
	style(SC_MARGIN_SYMBOL), width(0), mask(0), sensitive(false), cursor(SC_CURSORREVERSEARROW) {
}

// A list of the fontnames - avoids wasting space in each style
FontNames::FontNames() {
	size = 8;
	names = new char *[size];
	max = 0;
}

FontNames::~FontNames() {
	Clear();
	delete []names;
	names = 0;
}

void FontNames::Clear() {
	for (int i=0; i<max; i++) {
		delete []names[i];
	}
	max = 0;
}

const char *FontNames::Save(const char *name) {
	if (!name)
		return 0;
	for (int i=0; i<max; i++) {
		if (strcmp(names[i], name) == 0) {
			return names[i];
		}
	}
	if (max >= size) {
		// Grow array
		int sizeNew = size * 2;
		char **namesNew = new char *[sizeNew];
		for (int j=0; j<max; j++) {
			namesNew[j] = names[j];
		}
		delete []names;
		names = namesNew;
		size = sizeNew;
	}
	names[max] = new char[strlen(name) + 1];
	strcpy(names[max], name);
	max++;
	return names[max-1];
}

FontRealised::FontRealised(const FontSpecification &fs) {
	frNext = NULL;
	(FontSpecification &)(*this) = fs;
}

FontRealised::~FontRealised() {
	font.Release();
	delete frNext;
	frNext = 0;
}

void FontRealised::Realise(Surface &surface, int zoomLevel, int technology) {
	PLATFORM_ASSERT(fontName);
	sizeZoomed = size + zoomLevel * SC_FONT_SIZE_MULTIPLIER;
	if (sizeZoomed <= 2 * SC_FONT_SIZE_MULTIPLIER)	// Hangs if sizeZoomed <= 1
		sizeZoomed = 2 * SC_FONT_SIZE_MULTIPLIER;

	float deviceHeight = surface.DeviceHeightFont(sizeZoomed);
	FontParameters fp(fontName, deviceHeight / SC_FONT_SIZE_MULTIPLIER, weight, italic, extraFontFlag, technology, characterSet);
	font.Create(fp);

	ascent = surface.Ascent(font);
	descent = surface.Descent(font);
	aveCharWidth = surface.AverageCharWidth(font);
	spaceWidth = surface.WidthChar(font, ' ');
	if (frNext) {
		frNext->Realise(surface, zoomLevel, technology);
	}
}

FontRealised *FontRealised::Find(const FontSpecification &fs) {
	if (!fs.fontName)
		return this;
	FontRealised *fr = this;
	while (fr) {
		if (fr->EqualTo(fs))
			return fr;
		fr = fr->frNext;
	}
	return 0;
}

void FontRealised::FindMaxAscentDescent(unsigned int &maxAscent, unsigned int &maxDescent) {
	FontRealised *fr = this;
	while (fr) {
		if (maxAscent < fr->ascent)
			maxAscent = fr->ascent;
		if (maxDescent < fr->descent)
			maxDescent = fr->descent;
		fr = fr->frNext;
	}
}

ViewStyle::ViewStyle() {
	Init();
}

ViewStyle::ViewStyle(const ViewStyle &source) {
	frFirst = NULL;
	Init(source.stylesSize);
	for (unsigned int sty=0; sty<source.stylesSize; sty++) {
		styles[sty] = source.styles[sty];
		// Can't just copy fontname as its lifetime is relative to its owning ViewStyle
		styles[sty].fontName = fontNames.Save(source.styles[sty].fontName);
	}
	for (int mrk=0; mrk<=MARKER_MAX; mrk++) {
		markers[mrk] = source.markers[mrk];
	}
	CalcLargestMarkerHeight();
	for (int ind=0; ind<=INDIC_MAX; ind++) {
		indicators[ind] = source.indicators[ind];
	}

	selforeset = source.selforeset;
	selforeground = source.selforeground;
	selAdditionalForeground = source.selAdditionalForeground;
	selbackset = source.selbackset;
	selbackground = source.selbackground;
	selAdditionalBackground = source.selAdditionalBackground;
	selbackground2 = source.selbackground2;
	selAlpha = source.selAlpha;
	selAdditionalAlpha = source.selAdditionalAlpha;
	selEOLFilled = source.selEOLFilled;

	foldmarginColourSet = source.foldmarginColourSet;
	foldmarginColour = source.foldmarginColour;
	foldmarginHighlightColourSet = source.foldmarginHighlightColourSet;
	foldmarginHighlightColour = source.foldmarginHighlightColour;

	hotspotForegroundSet = source.hotspotForegroundSet;
	hotspotForeground = source.hotspotForeground;
	hotspotBackgroundSet = source.hotspotBackgroundSet;
	hotspotBackground = source.hotspotBackground;
	hotspotUnderline = source.hotspotUnderline;
	hotspotSingleLine = source.hotspotSingleLine;

	whitespaceForegroundSet = source.whitespaceForegroundSet;
	whitespaceForeground = source.whitespaceForeground;
	whitespaceBackgroundSet = source.whitespaceBackgroundSet;
	whitespaceBackground = source.whitespaceBackground;
	selbar = source.selbar;
	selbarlight = source.selbarlight;
	caretcolour = source.caretcolour;
	additionalCaretColour = source.additionalCaretColour;
	showCaretLineBackground = source.showCaretLineBackground;
	caretLineBackground = source.caretLineBackground;
	caretLineAlpha = source.caretLineAlpha;
	edgecolour = source.edgecolour;
	edgeState = source.edgeState;
	caretStyle = source.caretStyle;
	caretWidth = source.caretWidth;
	someStylesProtected = false;
	someStylesForceCase = false;
	leftMarginWidth = source.leftMarginWidth;
	rightMarginWidth = source.rightMarginWidth;
	for (int i=0; i < margins; i++) {
		ms[i] = source.ms[i];
	}
	maskInLine = source.maskInLine;
	fixedColumnWidth = source.fixedColumnWidth;
	zoomLevel = source.zoomLevel;
	viewWhitespace = source.viewWhitespace;
	whitespaceSize = source.whitespaceSize;
	viewIndentationGuides = source.viewIndentationGuides;
	viewEOL = source.viewEOL;
	extraFontFlag = source.extraFontFlag;
	extraAscent = source.extraAscent;
	extraDescent = source.extraDescent;
	marginStyleOffset = source.marginStyleOffset;
	annotationVisible = source.annotationVisible;
	annotationStyleOffset = source.annotationStyleOffset;
	braceHighlightIndicatorSet = source.braceHighlightIndicatorSet;
	braceHighlightIndicator = source.braceHighlightIndicator;
	braceBadLightIndicatorSet = source.braceBadLightIndicatorSet;
	braceBadLightIndicator = source.braceBadLightIndicator;
}

ViewStyle::~ViewStyle() {
	delete []styles;
	styles = NULL;
	delete frFirst;
	frFirst = NULL;
}

void ViewStyle::Init(size_t stylesSize_) {
	frFirst = NULL;
	stylesSize = 0;
	styles = NULL;
	AllocStyles(stylesSize_);
	fontNames.Clear();
	ResetDefaultStyle();

	// There are no image markers by default, so no need for calling CalcLargestMarkerHeight()
	largestMarkerHeight = 0;

	indicators[0].style = INDIC_SQUIGGLE;
	indicators[0].under = false;
	indicators[0].fore = ColourDesired(0, 0x7f, 0);
	indicators[1].style = INDIC_TT;
	indicators[1].under = false;
	indicators[1].fore = ColourDesired(0, 0, 0xff);
	indicators[2].style = INDIC_PLAIN;
	indicators[2].under = false;
	indicators[2].fore = ColourDesired(0xff, 0, 0);

	technology = SC_TECHNOLOGY_DEFAULT;
	lineHeight = 1;
	maxAscent = 1;
	maxDescent = 1;
	aveCharWidth = 8;
	spaceWidth = 8;

	selforeset = false;
	selforeground = ColourDesired(0xff, 0, 0);
	selAdditionalForeground = ColourDesired(0xff, 0, 0);
	selbackset = true;
	selbackground = ColourDesired(0xc0, 0xc0, 0xc0);
	selAdditionalBackground = ColourDesired(0xd7, 0xd7, 0xd7);
	selbackground2 = ColourDesired(0xb0, 0xb0, 0xb0);
	selAlpha = SC_ALPHA_NOALPHA;
	selAdditionalAlpha = SC_ALPHA_NOALPHA;
	selEOLFilled = false;

	foldmarginColourSet = false;
	foldmarginColour = ColourDesired(0xff, 0, 0);
	foldmarginHighlightColourSet = false;
	foldmarginHighlightColour = ColourDesired(0xc0, 0xc0, 0xc0);

	whitespaceForegroundSet = false;
	whitespaceForeground = ColourDesired(0, 0, 0);
	whitespaceBackgroundSet = false;
	whitespaceBackground = ColourDesired(0xff, 0xff, 0xff);
	selbar = Platform::Chrome();
	selbarlight = Platform::ChromeHighlight();
	styles[STYLE_LINENUMBER].fore = ColourDesired(0, 0, 0);
	styles[STYLE_LINENUMBER].back = Platform::Chrome();
	caretcolour = ColourDesired(0, 0, 0);
	additionalCaretColour = ColourDesired(0x7f, 0x7f, 0x7f);
	showCaretLineBackground = false;
	caretLineBackground = ColourDesired(0xff, 0xff, 0);
	caretLineAlpha = SC_ALPHA_NOALPHA;
	edgecolour = ColourDesired(0xc0, 0xc0, 0xc0);
	edgeState = EDGE_NONE;
	caretStyle = CARETSTYLE_LINE;
	caretWidth = 1;
	someStylesProtected = false;
	someStylesForceCase = false;

	hotspotForegroundSet = false;
	hotspotForeground = ColourDesired(0, 0, 0xff);
	hotspotBackgroundSet = false;
	hotspotBackground = ColourDesired(0xff, 0xff, 0xff);
	hotspotUnderline = true;
	hotspotSingleLine = true;

	leftMarginWidth = 1;
	rightMarginWidth = 1;
	ms[0].style = SC_MARGIN_NUMBER;
	ms[0].width = 0;
	ms[0].mask = 0;
	ms[1].style = SC_MARGIN_SYMBOL;
	ms[1].width = 16;
	ms[1].mask = ~SC_MASK_FOLDERS;
	ms[2].style = SC_MARGIN_SYMBOL;
	ms[2].width = 0;
	ms[2].mask = 0;
	fixedColumnWidth = leftMarginWidth;
	maskInLine = 0xffffffff;
	for (int margin=0; margin < margins; margin++) {
		fixedColumnWidth += ms[margin].width;
		if (ms[margin].width > 0)
			maskInLine &= ~ms[margin].mask;
	}
	zoomLevel = 0;
	viewWhitespace = wsInvisible;
	whitespaceSize = 1;
	viewIndentationGuides = ivNone;
	viewEOL = false;
	extraFontFlag = 0;
	extraAscent = 0;
	extraDescent = 0;
	marginStyleOffset = 0;
	annotationVisible = ANNOTATION_HIDDEN;
	annotationStyleOffset = 0;
	braceHighlightIndicatorSet = false;
	braceHighlightIndicator = 0;
	braceBadLightIndicatorSet = false;
	braceBadLightIndicator = 0;
}

void ViewStyle::CreateFont(const FontSpecification &fs) {
	if (fs.fontName) {
		for (FontRealised *cur=frFirst; cur; cur=cur->frNext) {
			if (cur->EqualTo(fs))
				return;
			if (!cur->frNext) {
				cur->frNext = new FontRealised(fs);
				return;
			}
		}
		frFirst = new FontRealised(fs);
	}
}

void ViewStyle::Refresh(Surface &surface) {
	delete frFirst;
	frFirst = NULL;
	selbar = Platform::Chrome();
	selbarlight = Platform::ChromeHighlight();

	for (unsigned int i=0; i<stylesSize; i++) {
		styles[i].extraFontFlag = extraFontFlag;
	}

	CreateFont(styles[STYLE_DEFAULT]);
	for (unsigned int j=0; j<stylesSize; j++) {
		CreateFont(styles[j]);
	}

	frFirst->Realise(surface, zoomLevel, technology);

	for (unsigned int k=0; k<stylesSize; k++) {
		FontRealised *fr = frFirst->Find(styles[k]);
		styles[k].Copy(fr->font, *fr);
	}
	maxAscent = 1;
	maxDescent = 1;
	frFirst->FindMaxAscentDescent(maxAscent, maxDescent);
	maxAscent += extraAscent;
	maxDescent += extraDescent;
	lineHeight = maxAscent + maxDescent;

	someStylesProtected = false;
	someStylesForceCase = false;
	for (unsigned int l=0; l<stylesSize; l++) {
		if (styles[l].IsProtected()) {
			someStylesProtected = true;
		}
		if (styles[l].caseForce != Style::caseMixed) {
			someStylesForceCase = true;
		}
	}

	aveCharWidth = styles[STYLE_DEFAULT].aveCharWidth;
	spaceWidth = styles[STYLE_DEFAULT].spaceWidth;

	fixedColumnWidth = leftMarginWidth;
	maskInLine = 0xffffffff;
	for (int margin=0; margin < margins; margin++) {
		fixedColumnWidth += ms[margin].width;
		if (ms[margin].width > 0)
			maskInLine &= ~ms[margin].mask;
	}
}

void ViewStyle::AllocStyles(size_t sizeNew) {
	Style *stylesNew = new Style[sizeNew];
	size_t i=0;
	for (; i<stylesSize; i++) {
		stylesNew[i] = styles[i];
		stylesNew[i].fontName = styles[i].fontName;
	}
	if (stylesSize > STYLE_DEFAULT) {
		for (; i<sizeNew; i++) {
			if (i != STYLE_DEFAULT) {
				stylesNew[i].ClearTo(styles[STYLE_DEFAULT]);
			}
		}
	}
	delete []styles;
	styles = stylesNew;
	stylesSize = sizeNew;
}

void ViewStyle::EnsureStyle(size_t index) {
	if (index >= stylesSize) {
		size_t sizeNew = stylesSize * 2;
		while (sizeNew <= index)
			sizeNew *= 2;
		AllocStyles(sizeNew);
	}
}

void ViewStyle::ResetDefaultStyle() {
	styles[STYLE_DEFAULT].Clear(ColourDesired(0,0,0),
	        ColourDesired(0xff,0xff,0xff),
	        Platform::DefaultFontSize() * SC_FONT_SIZE_MULTIPLIER, fontNames.Save(Platform::DefaultFont()),
	        SC_CHARSET_DEFAULT,
	        SC_WEIGHT_NORMAL, false, false, false, Style::caseMixed, true, true, false);
}

void ViewStyle::ClearStyles() {
	// Reset all styles to be like the default style
	for (unsigned int i=0; i<stylesSize; i++) {
		if (i != STYLE_DEFAULT) {
			styles[i].ClearTo(styles[STYLE_DEFAULT]);
		}
	}
	styles[STYLE_LINENUMBER].back = Platform::Chrome();

	// Set call tip fore/back to match the values previously set for call tips
	styles[STYLE_CALLTIP].back = ColourDesired(0xff, 0xff, 0xff);
	styles[STYLE_CALLTIP].fore = ColourDesired(0x80, 0x80, 0x80);
}

void ViewStyle::SetStyleFontName(int styleIndex, const char *name) {
	styles[styleIndex].fontName = fontNames.Save(name);
}

bool ViewStyle::ProtectionActive() const {
	return someStylesProtected;
}

bool ViewStyle::ValidStyle(size_t styleIndex) const {
	return styleIndex < stylesSize;
}

void ViewStyle::CalcLargestMarkerHeight() {
	largestMarkerHeight = 0;
	for (int m = 0; m <= MARKER_MAX; ++m) {
		switch (markers[m].markType) {
		case SC_MARK_PIXMAP:
			if (markers[m].pxpm->GetHeight() > largestMarkerHeight)
				largestMarkerHeight = markers[m].pxpm->GetHeight();
			break;
		case SC_MARK_RGBAIMAGE:
			if (markers[m].image->GetHeight() > largestMarkerHeight)
				largestMarkerHeight = markers[m].image->GetHeight();
			break;
		}
	}
}

