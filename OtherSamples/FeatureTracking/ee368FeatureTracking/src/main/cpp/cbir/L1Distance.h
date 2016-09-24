#ifndef L1_DISTANCE_H
#define L1_DISTANCE_H

class L1Distance {
public:
	template <class T>
	inline Float operator() (T &aDesc1, T &aDesc2) {
		Int size1 = aDesc1.size();

		Float dist = 0;
		
		for (Int i = 0; i < size1; ++i) {
			Float d = aDesc1[i] - aDesc2[i];
			if (d < 0) d = -d;

			dist += d;
		}
		return dist;
	}

	inline Int operator() (vector<Int> &aDesc1, vector<Int> &aDesc2) {
		Int size1 = aDesc1.size();

		Int dist = 0;
		
		for (Int i = 0; i < size1; ++i) {
			Int d = aDesc1[i] - aDesc2[i];
			if (d < 0) d = -d;

			dist += d;
		}
		return dist;
	}

	inline TFixed operator() (vector<TFixed> &aDesc1, vector<TFixed> &aDesc2) {
		static TFixed zero(0);
		Int size1 = aDesc1.size();

		TFixed dist = 0;
		
		for (Int i = 0; i < size1; ++i) {
			TFixed d = aDesc1[i] - aDesc2[i];
			if (d < zero) d = -d;

			dist += d;
		}
		return dist;
	}

	inline Int operator() (vector<Int> &aDesc1, vector<Int> &aDesc2, Int aBestDist) {
		Int size1 = aDesc1.size();

		Int dist = 0;
		for (Int i = 0; i < size1; ++i) {
			if (aDesc1[i] < aDesc2[i]) {
				dist += (aDesc2[i] - aDesc1[i]);
			} else {
				dist += (aDesc1[i] - aDesc2[i]);
			}

			if (dist >= aBestDist) break;
		}
		return dist;
	}

	template <class T>
	inline Float operator() (T &aDesc1, T &aDesc2, Float aBestDist) {
		Int size1 = aDesc1.size();

		Float dist = 0;
		for (Int i = 0; i < size1; ++i) {
			if (aDesc1[i] < aDesc2[i]) {
				dist += (aDesc2[i] - aDesc1[i]);
			} else {
				dist += (aDesc1[i] - aDesc2[i]);
			}

			if (dist >= aBestDist) break;
		}
		return dist;
	}
};

#endif
