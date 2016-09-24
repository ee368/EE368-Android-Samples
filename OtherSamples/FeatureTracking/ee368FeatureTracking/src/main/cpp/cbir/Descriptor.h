#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include "cbir/stl.h"
#include "cbir/types.h"
#include "cbir/print.h"
#include "cbir/TemplateTypes.h"

class Descriptor {
public:
	// construction methods
	Descriptor() {
	}
	Descriptor(Int aSize) {
		Resize(aSize);
	}
	~Descriptor() {
	}

	// methods
	Int size() {
		return iDescriptor.size();
	}
	Int Size() {
		return iDescriptor.size();
	}
	DescType Get(Int aIndex) {
		return iDescriptor[aIndex];
	}
	Bool IsEqualTo(Descriptor &aOther) {
		return iDescriptor == aOther.iDescriptor;
	}
	void Print() {
		Int size = Size();
		if (size == 0)
		{
			DPRINT("NULL\n");
			return;
		}

		for (Int i = 0; i < size; ++i) {
			DPRINTF(( "%f ", (Float)iDescriptor[i] ));
		}
	}
	void resize(Int aSize) {
		if (aSize >= 0) {
			iDescriptor.resize(aSize);
		}
	}
	void Resize(Int aSize) {
		if (aSize >= 0) {
			iDescriptor.resize(aSize);
		}
	}
	void SetZero() {
		Int size = Size();
		for (Int i = 0; i < size; i++) {
			iDescriptor[i] = 0;
		}
	}

	vector<DescType> &GetVector() {
		return iDescriptor;
	}

	// operators
	Descriptor &operator=(const Descriptor &aSrc) {		
		iDescriptor = aSrc.iDescriptor;
		return *this;
	}
	DescType &operator[] (Int aIndex) {
		return iDescriptor[aIndex];
	}
protected:
	vector<DescType> iDescriptor;
};

#endif
