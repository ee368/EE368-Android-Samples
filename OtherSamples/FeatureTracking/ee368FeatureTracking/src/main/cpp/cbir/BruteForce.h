#ifndef BRUTE_FORCE_H
#define BRUTE_FORCE_H

#include <time.h>

#include <queue>
#include "cbir/types.h"

template <class DescType, class DistType, class Dist>
class BruteForce {
public:
	BruteForce() {
		iBigNumber = numeric_limits<DistType>::max();
	}

public:
	void FindNN(	Int aK, 
			vector< vector<DescType> > &aQuery, 
			vector< vector<DescType> > &aDataBase, 
			vector< vector<Int> > &aNeighbors) {

		aNeighbors.resize(aQuery.size());
		for (Int i = 0; i < aQuery.size(); ++i) {
			FindNN(aK, aQuery[i], aDataBase, aNeighbors[i]);
		}
	}

	void FindNN(	Int aK, 
			vector< vector<DescType> > &aQuery, 
			vector< vector<DescType> > &aDataBase, 
			vector< vector<Int> > &aNeighbors,
			vector< vector<DistType> > &aDist) {

		aDist.resize(aQuery.size());
		aNeighbors.resize(aQuery.size());
		for (Int i = 0; i < aQuery.size(); ++i) {
			FindNN(aK, aQuery[i], aDataBase, aNeighbors[i], aDist[i]);
		}
	}

	void FindNN(	Int aK, 
			vector<DescType> &aQuery, 
			vector< vector<DescType> > &aDataBase, 
			vector<Int> &aNeighbors) {

		vector<DistType> dist;
		FindNN(aK, aQuery, aDataBase, aNeighbors, dist);
	}

	void FindNN(	Int aK, 
			vector<DescType> &aQuery, 
			vector< vector<DescType> > &aDataBase, 
			vector<Int> &aNeighbors,
			vector<DistType> &aDist) {

		// initialize
		priority_queue< pair<DistType, Int> > bestPoints;
		pair<DistType, Int> initPair(iBigNumber, 0);
		bestPoints.push(initPair);

		// linear search
		Int dbSize = aDataBase.size();
		for (Int i = 0; i < dbSize; ++i) {
			pair<DistType, Int> top = bestPoints.top();

			// early terminating distance
			DistType dist = iDist(aDataBase[i], aQuery, top.first);

			if (dist < top.first) {
				pair<DistType, Int> distPair(dist, i);
				bestPoints.push(distPair);
				while ((Int) bestPoints.size() > aK) bestPoints.pop();
			}

#if 1
// hack to give other threads time
if (i % 1000 == 0) {
struct timespec dur, rem;
dur.tv_sec = 0;
dur.tv_nsec = 1000000;	// 1 ms
nanosleep(&dur, &rem);
}
#endif
		}

		// copy results
		aDist.resize(aK);
		aNeighbors.resize(aK);
		for (Int i = aK-1; i >= 0; --i) {
			pair<DistType, Int> top = bestPoints.top();
			aDist[i] = top.first;
			aNeighbors[i] = top.second;
			bestPoints.pop();
		}
	}
	
public:
	Dist iDist;

	DistType iBigNumber;
};

#endif
