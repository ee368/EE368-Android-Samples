#ifndef RING_BUFFER_H
#define RING_BUFFER_H

template <class T>
class RingBuffer{
public:
	RingBuffer() {
		iStart = 0;
	}
	RingBuffer(Int aLen) {
		Construct(aLen);
	}
	void Construct(Int aLen) {
		iStart = 0;
		iNextStart = 0;
		iSize = 0;
		iRing.resize(0);
		iRing.resize(aLen);
	}
	
	Int MaxSize() { return iRing.size(); }
	Int Size() { return iSize; }

	void Push(T &aInput) {
		iStart = iNextStart;
		if (iSize < MaxSize()) ++iSize;

		Int idx = iStart + iSize - 1;
//		idx %= MaxSize();
		idx = Mod(idx, MaxSize());

		iRing[idx] = aInput;

//		if (iSize == MaxSize()) iNextStart = (iStart+1) % MaxSize();
		if (iSize == MaxSize()) iNextStart = Mod( (iStart+1), MaxSize() );
	}
	T &operator[](Int aIndex) {
		Int idx = aIndex + iStart;
//		idx %= MaxSize();
		idx = Mod(idx, MaxSize());
		return iRing[idx];
	}

	template<class S>
	inline S Mod(S x, S m) {
		return x % m;
	}
private:
	Int iStart;
	Int iNextStart;
	Int iSize;
	vector<T> iRing;
};

#endif
