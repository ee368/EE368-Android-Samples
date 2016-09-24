#ifndef FONT_H
#define FONT_H

#include "cbir/types.h"

class Glyphs {
public:
	const static Int iNumChar = 256;
	const static Int iFontWidth = 16;
	const static Int iFontHeight = 16;
	Bool iGlyphs[256][256];

public:
	Glyphs() {
		#include "cbir/Glyphs.h"

		Int charLen = iFontWidth * iFontHeight;

		for (Int i = 0; i < iNumChar; ++i) {
			for (Int j = 0; j < charLen; ++j) {
				iGlyphs[i][j] = glyphs[i][j];
			}
		}
	}

	Bool &operator() (Int aChar, Int aPixel) {
		return iGlyphs[aChar][aPixel];
	}
};

#endif

