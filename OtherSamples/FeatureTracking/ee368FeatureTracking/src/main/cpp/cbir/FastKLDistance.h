#ifndef FAST_KL_DISTANCE_H
#define FAST_KL_DISTANCE_H

#include "cbir/Descriptor.h"
#include <math.h>

class FastKLDistance {
public:
	template <class T>
	Float operator() (T &aDesc1, T &aDesc2) {
		Int size1 = aDesc1.size();

		Float dist = 0;
		
		for (Int i = 0; i < size1; ++i) {
			Float logRatio = FastLog2(aDesc1[i] / aDesc2[i]);

			Float d1 = aDesc1[i] * logRatio;
			Float d2 = aDesc2[i] * -logRatio;

			Float d = d1 + d2;
			dist += d;
		}
		return dist;
	}
	template <class T>
	Float operator() (T &aDesc1, T &aDesc2, Float aBestDist) {
		Int size1 = aDesc1.size();

		Float dist = 0;
		
		for (Int i = 0; i < size1; ++i) {
			Float logRatio = FastLog2(aDesc1[i] / aDesc2[i]);

			Float d1 = aDesc1[i] * logRatio;
			Float d2 = aDesc2[i] * -logRatio;

			Float d = d1 + d2;
			dist += d;
			if (dist >= aBestDist) break;
		}
		return dist;
	}

	static inline float FastLog2 (float val) {
		union { 
			float f; 
			int i; 
		} conv;

		// convert float to int for access to bits
		conv.f = val;
		int x = conv.i;

		// extract exponent
		int log2 = ((x >> 23) & 0xff) - 128;

		// clear exponent
		x &= ~(0xff << 23);

		// set exponent to zero
		x += 127 << 23;
		
		// convert 
		conv.i = x;
		val = conv.f;

		// approximate log with polynomial
		val = (-0.33333333 * val + 2) * val - 0.66666667;

		return (val + log2);
	} 

public:
	const static Float KLog2Inv = 1.44269504088896;
};

#endif
