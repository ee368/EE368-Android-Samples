#ifndef RANSAC_H
#define RANSAC_H

#include "cbir/types.h"
#include "cbir/AffineSolver.h"

#include <stdlib.h>

class Ransac {
public:
	Ransac() { Init(); }
	void Init() {
		iMaxIter = 10;
		iMinMatches = 4;
		iMinInliers = 4;
		iDistThresh = 3*3;
	}

	// perform ransac
	template <class T>	// T must be vector< vector<FrameType> > or FrameArray
	void Verify(	T &aFrames1,
			T &aFrames2,
			vector< pair<Int,Int> > &aMatches,
			vector<Int> &aInliers,
			vector<Float> &aModel) {

		Int numMatches = aMatches.size();

		// require a minimum number of matches
		if (numMatches < iMinMatches) return;

		// main iteration loop
		AffineSolver affine;
		Int numStart = 3;
		vector<Int> indices(numStart);
		vector<Int> bestInliers;
		vector<Float> model, bestModel;
		for (Int i = 0; i < iMaxIter; ++i) {
			// choose random indices
			RandomIndices(indices, numMatches);

			// compute model
			for (Int j = 0; j < numStart; ++j) {
				Int idx = indices[j];
				pair<Int,Int> &match = aMatches[idx];
				affine.AddMatch(aFrames2[match.second][KX], aFrames2[match.second][KY], 
						aFrames1[match.first][KX], aFrames1[match.first][KY]);
			}
			affine.ComputeTransform(model);

			if (model.size() == 0) continue;

			// find inliers
			vector<Int> inliers;
			vector<Float> p1(3), p2(3);
			p1[2] = 1;
			p2[2] = 1;
			for (Int j = 0; j < numMatches; ++j) {
				pair<Int,Int> &match = aMatches[j];
				p1[0] = aFrames1[match.first][KX];
				p1[1] = aFrames1[match.first][KY];

				p2[0] = aFrames2[match.second][KX];
				p2[1] = aFrames2[match.second][KY];

				vector<Float> q = affine.Transform(model, p2);

				Float dx = p1[0]-q[0];
				Float dy = p1[1]-q[1];
				Float dist = dx*dx + dy*dy;

				if (dist < iDistThresh) {
					inliers.push_back(j);
				}
			}

			// update best inliers
			if (inliers.size() > bestInliers.size() && 
					inliers.size() > (Uint) iMinInliers) {
				bestInliers = inliers;
				bestModel = model;
			}
		}

		// set return parameters
		aInliers = bestInliers;
		aModel = bestModel;
	}

private:
	inline void RandomIndices(vector<Int> &aIndices, Int aMax) {
		Int len = aIndices.size(); 
		for (Int i = 0; i < len; ++i) {
			aIndices[i] = RandomIndex(aMax);
		}
	}
	inline Int RandomIndex(Int aMax) {
		Int idx = rand() % aMax;
		return idx;
	}

public:
	Int iMinMatches;
	Int iMinInliers;
	Int iMaxIter;
	Int iDistThresh;

private:
	static const Int KX = 0;
	static const Int KY = 1;
	static const Int KScl = 2;
	static const Int KOri = 3;
	static const Int KRes = 4;
};

#endif
