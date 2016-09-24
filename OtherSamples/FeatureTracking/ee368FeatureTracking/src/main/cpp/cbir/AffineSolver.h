#ifndef AFFINE_SOLVER
#define AFFINE_SOLVER

// direct solve of an affine system via the normal equations

// single sample: Ax = y
// multi samples: AX = Y
// solve for A
// AXX' = YX'		maintain XX' and YX'
// A = YX'(XX')^-1	invert XX'

#include <cbir/stl.h>
#include "cbir/types.h"

class AffineSolver {
public:
	// Constructor 
	AffineSolver() {
		Zero();
	}

	void Zero() {
		w = 0;
		sx = 0;
		sy = 0;		
		dx = 0;
		dy = 0;		
		sxsx = 0;
		sysy = 0;
		sxsy = 0;		
		sxdx = 0;
		sxdy = 0;
		sydx = 0;
		sydy = 0;
	}

	// Add a matching point to the solver 
	void AddMatch(Float srcX, Float srcY, Float dstX, Float dstY, Float weight = 1) {
		w += weight;
		sx += weight * srcX;	
		sy += weight * srcY;
		dx += weight * dstX;
		dy += weight * dstY;
		sxsx += weight * srcX * srcX;
		sysy += weight * srcY * srcY;
		sxsy += weight * srcX * srcY;		
		sxdx += weight * srcX * dstX;
		sxdy += weight * srcX * dstY;
		sydx += weight * srcY * dstX;
		sydy += weight * srcY * dstY;
	}
    
	// Execute the solver 
	Bool ComputeTransform(vector<Float> &transform) const {
		// to get the affine transform:
		// (a b c)
		// (d e f)
		// we need to solve:
		// (sxsx sxsy sx)   (a d)   (sxdx sxdy)
		// (sxsy sysy sy) * (b e) = (sydx sydy)
		// ( sx   sy  w )   (c f)   ( dx   dy )

		// this function will probably compile to slow emulated floating point unless you do something clever

		// compute the unscaled inverse, it's symettric
		Float m11 = w*sysy    - sy*sy;
		Float m12 = sy*sx     - w*sxsy;
		Float m13 = sxsy*sy   - sx*sysy;
		Float m22 = w*sxsx    - sx*sx;
		Float m23 = sxsy*sx   - sy*sxsx;
		Float m33 = sxsx*sysy - sxsy*sxsy;	
       
		// compute the determinant
		Float det = sxsx*m11 + sxsy*m12 + sx*m13;

		// check it's not degenerate
		// should probably do something other than just return
		if (!det) return false;

		transform.resize(9);

		// compute the unscaled affine output
		transform[0] = m11 * sxdx + m12 * sydx + m13 * dx;
		transform[1] = m12 * sxdx + m22 * sydx + m23 * dx;
		transform[2] = m13 * sxdx + m23 * sydx + m33 * dx;
		transform[3] = m11 * sxdy + m12 * sydy + m13 * dy;
		transform[4] = m12 * sxdy + m22 * sydy + m23 * dy;
		transform[5] = m13 * sxdy + m23 * sydy + m33 * dy;

		// scale it
		transform[0] /= det;
		transform[1] /= det;
		transform[2] /= det;
		transform[3] /= det;
		transform[4] /= det;
		transform[5] /= det;

		// last row
		transform[6] = 0;
		transform[7] = 0;
		transform[8] = 1;

		return true;
	}

	static vector<Float> Transform(vector<Float> &A, vector<Float> &x) {
		vector<Float> y(3);

		y[0] = A[0]*x[0] + A[1]*x[1] + A[2]*x[2];
		y[1] = A[3]*x[0] + A[4]*x[1] + A[5]*x[2];
		y[2] = A[6]*x[0] + A[7]*x[1] + A[8]*x[2];

		return y;
	}

	static vector<Float> AffineMultiply(vector<Float> A, vector<Float> B) {
		Int dim = 9;
		vector<Float> C(dim);

		C[0] = A[0]*B[0] + A[1]*B[3] + A[2]*B[6];
		C[1] = A[0]*B[1] + A[1]*B[4] + A[2]*B[7];
		C[2] = A[0]*B[2] + A[1]*B[5] + A[2]*B[8];

		C[3] = A[3]*B[0] + A[4]*B[3] + A[5]*B[6];
		C[4] = A[3]*B[1] + A[4]*B[4] + A[5]*B[7];
		C[5] = A[3]*B[2] + A[4]*B[5] + A[5]*B[8];

		C[6] = A[6]*B[0] + A[7]*B[3] + A[8]*B[6];
		C[7] = A[6]*B[1] + A[7]*B[4] + A[8]*B[7];
		C[8] = A[6]*B[2] + A[7]*B[5] + A[8]*B[8];

		return C;
	}

	static vector<Float> Eye() {
		vector<Float> A(9);

		for (Int i = 0; i < 9; ++i) {
			A[i] = 0;
		}

		A[0] = 1;
		A[4] = 1;
		A[8] = 1;

		return A;
	}

private:
    Float sx, sxsx, sy, sysy, sxsy, w, sxdx, sxdy, sydx, sydy, dx, dy;
};

#endif

