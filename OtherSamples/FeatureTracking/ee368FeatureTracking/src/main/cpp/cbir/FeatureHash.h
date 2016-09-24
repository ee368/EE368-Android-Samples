#ifndef FEATURE_HASH_H
#define FEATURE_HASH_H

class FeatureHash {
public:
	FeatureHash() {
		iWidth = 0;
		iHeight = 0;
		iBinSize = 1;
		iLog2BinSize = 0;
		iTableWidth = 0;
		iTableHeight = 0;
	}
	FeatureHash(Int aWidth, Int aHeight, Int aBinSize, FeatureStore &aFeatureStore) {
		Construct(aWidth, aHeight, aBinSize, aFeatureStore);
	}
	void Construct(Int aWidth, Int aHeight, Int aBinSize, FeatureStore &aFeatureStore) {
		iWidth = aWidth;
		iHeight = aHeight;
		iBinSize = aBinSize;
		iLog2BinSize = (Int) ceil(log(iBinSize) / log(2));

		iTableWidth = (Int) (iWidth >> iLog2BinSize) + 1;
		iTableHeight = (Int) (iHeight >> iLog2BinSize) + 1;

		// allocate table
		iHashTable.resize(iTableWidth);
		for (Int i = 0; i < iTableWidth; ++i) {
			iHashTable[i].resize(iTableHeight);
			for (Int j = 0; j < iTableHeight; ++j) {
				iHashTable[i][j].resize(0);
			}
		}

		// fill table
		Int numFrames = aFeatureStore.Size();
		for (Int k = 0; k < numFrames; ++k) {
			Frame &frame = aFeatureStore.GetFrame(k);
			
			Int xBin = Int(frame[KX]) >> iLog2BinSize;
			Int yBin = Int(frame[KY]) >> iLog2BinSize;

			// store index of feature in neighboring bins
			for (Int i = -1; i <= 1; ++i) {
				for (Int j = -1; j <= 1; ++j) {
					Int ii = xBin + i;
					Int jj = yBin + j;

					// check bounds
					if (ii < 0) continue;
					if (jj < 0) continue;
					if (ii >= iTableWidth) continue;
					if (jj >= iTableHeight) continue;

					// store index
					iHashTable[ii][jj].push_back(k);
				}
			}
		}
	}
	vector<Int> &GetNeighbors(Frame &aFrame) {
		Int xBin = Int(aFrame[KX]) >> iLog2BinSize;
		Int yBin = Int(aFrame[KY]) >> iLog2BinSize;

		return iHashTable[xBin][yBin];
	}
public:
	template<class T>
	static inline T Floor(T x) {
		if (x < 0) return (T) Int(x-1);
		return (T) Int(x);
	}
	template<class T>
	static inline T Round(T x) {
		return (T) Floor(x + 0.5);
	}
	template<class T>
	static inline T Max(T a, T b) {
		if (a < b) return b;
		return a;
	}
	template<class T>
	static inline T Min(T a, T b) {
		if (a < b) return a;
		return b;
	}
	template<class T>
	static inline T Mod(T x, T m) {
		while (x < 0) x += m;
		while (x >= m) x -= m;
		return x;
	}

public:
	Int iWidth;
	Int iHeight;
	Int iBinSize;
	Int iLog2BinSize;
	Int iTableWidth;
	Int iTableHeight;
	vector< vector< vector<Int> > > iHashTable;
private:
	const static Int KX = 0;
	const static Int KY = 1;
	const static Int KScl = 2;
	const static Int KOri = 3;
	const static Int KRes = 4;
};

#endif
