#ifndef RIFF_PAIR_H
#define RIFF_PAIR_H
// impelmentation of stl-like pair for port to android

namespace riff {

template <class S, class T>
class pair {
public:
	pair() {
	}
	pair(S aFirst, T aSecond) {
		first = aFirst;
		second = aSecond;
	}
	pair(const pair &aPair) {
		first = aPair.first;
		second = aPair.second;
	}
	~pair() {
	}

public:
	pair &operator= (const pair &aPair) {
		first = aPair.first;
		second = aPair.second;
		return *this;
	}
	template <class A, class B> friend bool operator< (const pair<A,B> &aPair1, const pair<A,B> &aPair2);
	template <class A, class B> friend bool operator== (const pair<A,B> &aPair1, const pair<A,B> &aPair2);
public:
	S first;
	T second;
};

// comparison operators
template <class S, class T>
bool operator< (const pair<S,T> &aPair1, const pair<S,T> &aPair2) {
	if (aPair1.first < aPair2.first) return true;
	if (aPair1.first > aPair2.first) return false;
	if (aPair1.second < aPair2.second) return true;
	return false;
}

template <class S, class T>
bool operator== (const pair<S,T> &aPair1, const pair<S,T> &aPair2) {
	if (aPair1.first != aPair2.first) return false;
	if (aPair1.second != aPair2.second) return false;
	return true;
}

}
#endif
