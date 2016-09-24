#ifndef QUANTIZERS_H
#define QUANTIZERS_H

#include "cbir/types.h"
#include "cbir/Fixed.h"

class Quantize3x3 {
public:
	const static Int iNumBins = 9;
	const static Int iSqrtNumBins = 3;

public:
	inline Int operator() (TFixed dr, TFixed dt) {
		Int idxR = ScalarQuantize(dr);
		Int idxT = ScalarQuantize(dt);

		// 3x3 grid
		Int idx = 3 * idxR + idxT;

		return idx;
	}

	inline Int ScalarQuantize(TFixed x) {
		// quantize to levels [-0.5 -0.25 0 0.25 0.5]
		x <<= 2;	// multiply by 4
		Int idx = x.Round();
		if (idx >= 1) return 2;
		if (idx <= -1) return 0;
		return 1;
	}
};

class Quantize5x5 {
public:
	const static Int iNumBins = 25;
	const static Int iSqrtNumBins = 5;

public:
	inline Int operator() (TFixed dr, TFixed dt) {
		Int idxR = ScalarQuantize(dr);
		Int idxT = ScalarQuantize(dt);

		// 5 by 5 grid
		Int idx = 5 * idxR + idxT;

		return idx;
	}

	inline Int ScalarQuantize(TFixed x) {
		// quantize to levels [-0.5 -0.25 0 0.25 0.5]
		x <<= 2;	// multiply by 4
		Int idx = x.Round();
		if (idx > 2) return 4;
		if (idx < -2) return 0;
		return idx + 2;
	}
};


#endif
