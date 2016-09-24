#ifndef CELL_MAP_H
#define CELL_MAP_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

#include "cbir/stl.h"
#include "cbir/types.h"
#include "cbir/print.h"
#include "cbir/Image.h"

class CellMap {
public:
	CellMap() {
		iPatchSize = 0;
		iNumCells = 0;
	}
	CellMap(Char *aFile) {
		Construct(aFile);
	}

	void Construct(Char *aFile) {
		// defaults to Annuli4Patch35Skip
		Int patchSize = 35;
		Int numRadii = 4;
		Int numAngles = 1;
		Float radiusOverlap = 0.0;
		Float angleOverlap = 0.0;
		Float exponent = 1.2;
		Int skip = 1;

		if (!strcmp(aFile, "Annuli4Patch35Skip")) {
			patchSize = 35;
			numRadii = 4;
			numAngles = 1;
			radiusOverlap = 0.0;
			angleOverlap = 0.0;
			exponent = 1.2;
			skip = 1;
		} else if (!strcmp(aFile, "Annuli4Patch35")) {
			patchSize = 35;
			numRadii = 4;
			numAngles = 1;
			radiusOverlap = 0.0;
			angleOverlap = 0.0;
			exponent = 1.2;
			skip = 0;
		} else if (!strcmp(aFile, "Polar3x6Patch31Skip")) {
			patchSize = 31;
			numRadii = 3;
			numAngles = 6;
			radiusOverlap = 0.0;
			angleOverlap = 0.0;
			exponent = 1.2;
			skip = 1;
		} else if (!strcmp(aFile, "Polar3x6Patch31")) {
			patchSize = 31;
			numRadii = 3;
			numAngles = 6;
			radiusOverlap = 0.0;
			angleOverlap = 0.0;
			exponent = 1.2;
			skip = 0;
		}

		ConstructPolarMap(patchSize, 
				numRadii, numAngles, 
				radiusOverlap, angleOverlap,
				exponent, skip);

		//DrawCellMap();
	}

	void DrawCellMap() {
		Image<Byte> image(iPatchSize, iPatchSize, 1);
		
		for (Int x = 0; x < iPatchSize; ++x) {
			for (Int y = 0; y < iPatchSize; ++y) {
				Int size = iMap[x][y].size();
				for (Int i = 0; i < size; ++i) {
					image(x, y) += iMap[x][y][i]+1;
				}
			}
		}

		ImageIO::WritePGM("cellMap.pgm", image);
	}

	void ConstructPolarMap(
			Int aPatchSize, 
			Int aNumRadii, 
			Int aNumAngles, 
			Float aRadiusOverlap = 0,
			Float aAngleOverlap = 0,
			Float aExponent = 1.0,
			Int aSkip = 0) {

		// number of cells for this configuration
		iNumCells = (aNumRadii-1) * aNumAngles + 1;

		// clear cells
		iCells.resize(iNumCells);
		for (Int i = 0; i < iNumCells; ++i) {
			iCells[i].clear();
		}

		// initialize map
		iPatchSize = aPatchSize;
		iMap.clear();
		iMap.resize(iPatchSize);
		for (Int i = 0; i < iPatchSize; ++i) {
			iMap[i].resize(iPatchSize);
		}

		// patch geometry
		Float center = (iPatchSize-1) / 2.0;
		Float halfSize = iPatchSize - center;

		// loop over pixels in patch
		iNumPixels = 0;
		for (Int i = 0; i < iPatchSize; ++i) {
			for (Int j = 0; j < iPatchSize; ++j) {
				// compute polar coordinates
				Float x = i - center;
				Float y = j - center;

				Float radius = sqrt( x*x + y*y ) / halfSize;
				radius = pow(radius, aExponent);

				Float theta = atan2(y, x) / KPi / 2.0;
				theta = 1.0 - fmod((double)theta, (double)1.0);

				// loop over cells
				Int curCell = 0;
				for (Int m = 0; m < aNumRadii; ++m) {
					for (Int n = 0; n < aNumAngles; ++n) {
						Bool inAngleBin = false;
						Bool inRadiusBin = false;

						// skip pixels
						if (aSkip) {
							if ( (i+j) % (aSkip + 1) ) {
								continue;
							}
						}

						// special handling of inner cell
						if (m == 0 && n > 0) break;
						if (m == 0 && n == 0) {
							inAngleBin = true;
						} else {
							// compute distance from angle bin center
							Float centerTheta = Float(n) / aNumAngles;
							Float d0 = fabs(theta - centerTheta);
							Float d1 = fabs(theta-1 - centerTheta);
							Float angleDist = Min(d0, d1);

							// compute size of bin
							Float angleDistThresh = 0.5 / aNumAngles * (1 + aAngleOverlap);

							// are we in the bin
							if (angleDist <= angleDistThresh) {
								inAngleBin = true;
							}
						}

						// compute radial distance from center of bin
						Float centerRadius = Float(m) / aNumRadii;
						Float radiusDist = fabs(radius - centerRadius);
						
						// size of bin
						Float radiusDistThresh = 0.5 / aNumRadii * (1 + aRadiusOverlap);

						// are we in the bin
						if (radiusDist <= radiusDistThresh) {
							inRadiusBin = true;
						}

						// if in both radial and angle bins, then we are in cell
						if (inAngleBin && inRadiusBin) {
							iMap[i][j].push_back(curCell);

							ns::pair<Int,Int> xy(i, j);
							iCells[curCell].push_back(xy);

							++iNumPixels;
						}

						// increment cell counter
						++curCell;
					}
				}
			}
		}
	}

	virtual ~CellMap() {}

	Int Size() { return iNumCells; }
	Int size() { return iNumCells; }

	// operator
	inline vector<Int> &operator() (Int aX, Int aY) {
		return iMap[aX][aY];
	}
	inline vector< ns::pair<Int,Int> > &operator[] (Int aIndex) {
		return iCells[aIndex];
	}

	// simple math operators
	template<class T>
	inline T Floor(T x) {
		if (x < 0) return (T) Int(x-1);
		return (T) Int(x);

	}
	template<class T>
	inline T Round(T x) {
		return (T) Floor(x + 0.5);
	}
	template<class T>
	inline T Max(T a, T b) {
		if (a < b) return b;
		return a;
	}
	template<class T>
	inline T Min(T a, T b) {
		if (a < b) return a;
		return b;
	}
	template<class T>
	inline T Mod(T x, T m) {
		while (x < 0) x += m;
		while (x >= m) x -= m;
		return x;
	}

public:
	vector< vector< vector<Int> > > iMap;
	vector < vector< ns::pair<Int,Int> > > iCells;
	
	Int iPatchSize;
	Int iNumCells;
	Int iNumPixels;

	static const Float KPi = 3.14159265358979;
};

#endif
