#ifndef RIFF_VECTOR_H
#define RIFF_VECTOR_H
// impelmentation of stl-like vector for port to android

template <class T>
class vector {
public:
	vector() {
		Zero();
		Construct(0); 
	}
	vector(unsigned int aSize) {
		Zero();
		Construct(aSize);
	}
	void Zero() {
		iArray = 0;
		iSize = 0;
		iAllocSize = 0;
	}
	void Construct(unsigned int aSize) {
		Destruct();

		iSize = aSize;
		iAllocSize = aSize;
		if (aSize > 0) {
			iArray = new T[iAllocSize];
		} else {
			iArray = 0;
		}
	}
	void Destruct() {
		if (iArray) delete[] iArray;
		Zero();
	}
public:
	T *begin() { return &iArray[0]; }
	T *end() { return (&iArray[iSize-1])+1; }

	typedef T* iterator;
	typedef const T* const_iterator;

public:
	void resize(unsigned int aSize) {
		if (iAllocSize < aSize) {
			alloc(aSize);
		}
		iSize = aSize;
	}
	unsigned int size() const {
		return iSize;
	}

	unsigned int alloc_size() const {
	  return iAllocSize;
	}

	void pop_back() {
		if (iSize > 0) --iSize;
	}
	void push_back(const T& aElement) {
		if (iAllocSize == 0) {
			alloc(1);
		} else if (iSize >= iAllocSize) {
			alloc(2*iAllocSize);
		}
		
		iArray[iSize] = aElement;
		++iSize;
	}

	void clear() {
		if (iArray) delete [] iArray;
		iArray = 0;
		iSize = 0;
		iAllocSize = 0;
	}

	bool empty() {
		return iSize == 0;
	}

	iterator erase(iterator target) {
		if (iSize == 0) return end();

		// search for target
		iterator iter = begin();
		iterator iterEnd = end();
		unsigned int pos = 0;
		while ( (iter != target) && (iter != iterEnd) ) {
			++iter;
			++pos;
		}

		// target not found
		if (iter == iterEnd) return iter;

		// target found
		for (unsigned int i = pos+1; i < iSize; ++i) {
			iArray[i-1] = iArray[i];
		} // i
		--iSize;

		if (pos == iSize) {
			return end();
		} else {
			return iter;
		}
	}

public:
	T &operator[] (unsigned int aPos) const {
		return iArray[aPos];
	}

	vector<T>& operator= (const vector<T>& aVector) {
		// do not copy self
		if (this == &aVector) return *this;

		if (iAllocSize < aVector.size()) {
			alloc(aVector.size());
		}

		for (unsigned int i = 0; i < aVector.size(); ++i) {
			iArray[i] = aVector[i];
		}
		iSize = aVector.size();

		return *this;
	}

	bool operator== (const vector<T>& aVector) const {
		// check if self
		if (this == &aVector) return true;
	  
		// check if sizes different
		if (iSize != aVector.size()) return false;
	  
		// check element by element
		for (unsigned int i = 0; i < aVector.size(); ++i) {
			if (iArray[i] != aVector[i]) return false;
		}
	  
		return true;
	}

private:
	// alloc more memory without changing the external array size
	void alloc(unsigned int aSize) {
		iAllocSize = aSize;
		if (iArray) {
			T* iArrayOld = iArray;
			iArray = new T[iAllocSize];
			for (unsigned int i = 0; i < iSize; ++i) {
				iArray[i] = iArrayOld[i];
			}
			delete[] iArrayOld;
		} else {
			iArray = new T[iAllocSize];
		}
	}

private:
	T *iArray;
	unsigned int iSize;
	unsigned int iAllocSize;
};


#endif
