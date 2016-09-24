#ifndef FRAME_H 
#define FRAME_H

#include "cbir/stl.h"
#include "cbir/TemplateTypes.h"

class Frame {
public:
	// construction methods
	Frame() {
	}
	Frame(Int aSize) {
		Resize(aSize);
	}
	~Frame() {
	}

	// methods
	Int Size() {
		return iFrame.size();
	}
	void Copy(Frame aSrc) {
		iFrame = aSrc.iFrame;
	}
	FrameType Get(Int aIndex) {
		return iFrame[aIndex];
	}
	bool IsEqualTo(Frame &aOther) {
		return iFrame == aOther.iFrame;
	}
	void Print() {
		Int size = Size();
		for (Int i = 0; i < size; ++i) {
			DPRINTF(( "%0.2f ", (Float)iFrame[i] ));
		}
	}
	void Resize(Int aSize) {
		if (aSize >= 0) {
			iFrame.resize(aSize);
		}
	}

	virtual vector<FrameType> &GetVector() {
		return iFrame;
	}

	// operators
	FrameType &operator[] (Int aIndex) {
		return iFrame[aIndex];
	}
protected:
	vector<FrameType> iFrame;
};

#endif
