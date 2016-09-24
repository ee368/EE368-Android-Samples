#ifndef FRAME_ARRAY_H
#define FRAME_ARRAY_H

#include <stdlib.h>
#include "cbir/Frame.h"

class FrameArray {
public:
	// constructor and destructors
	FrameArray() {
	}
	~FrameArray() {
	}

	// accessors
	Frame *Get(Int aIndex) {
		return &iFrames[aIndex];
	}
	void Append(FrameArray &aFrames) {
		Int size = aFrames.Size();
		for (Int i = 0; i < size; ++i) {
			Append(aFrames[i]);
		}
	}
	void Append(Frame &aFrame) {
		iFrames.push_back(aFrame);
	}
	Int Size() {
		return iFrames.size();
	}
	Int Dimension() {
		if (iFrames.empty()) return 0;
		return iFrames[0].Size();
	}
	void Print() {
		Int size = Size();
		for (Int i = 0; i < size; ++i) {
			iFrames[i].Print();
			DPRINT("\n");
		}
	}
	
	void Resize(Int aSize) {
		if (aSize >= 0) iFrames.resize(aSize);
	}

	void Sort(Int aDim) {
		Int len = iFrames.size();
		if (aDim < 0 || aDim >= len) return;		// do nothing

		// copy the desired dimension into a pair, and sort
		// the second item in the pair is the index
		vector< ns::pair<FrameType, Int> > v(len);

		for (Int i = 0; i < len; ++i) {
			v[i].first = iFrames[i][aDim];
			v[i].second = i;
		}

		// do the sort
#ifdef DISABLE_STL
		qsort(v.begin(), len, sizeof(ns::pair<FrameType, Int>), comparePairs);
#else
		sort(v.begin(), v.end());
#endif

		// copy frames to new sorted array
		vector<Frame> sortedFrames(len);
		for (Int i = 0; i < len; ++i) {
			sortedFrames[i] = iFrames[ v[i].second ];
		}

		iFrames = sortedFrames;
	}

	static int comparePairs(const void* aPointer1, const void* aPointer2) {
		FrameType first1 = ((ns::pair<FrameType,Int>*)aPointer1)->first;
		FrameType first2 = ((ns::pair<FrameType,Int>*)aPointer2)->first;

		FrameType second1 = ((ns::pair<FrameType,Int>*)aPointer1)->second;
		FrameType second2 = ((ns::pair<FrameType,Int>*)aPointer2)->second;

		if (first1 < first2) {
			return -1;
		} else if (first1 == first2) {
			if (second1 < second2) {
				return -1;
			} else if (second1 == second2) {
				return 0;
			} else {
				return 1;
			}
		} else {
			return 1;
		}
	}

	// operators
	Frame &operator[] (Int aIndex) {
		return iFrames[aIndex];
	}
protected:
	vector< Frame > iFrames;
};

#endif
