#ifndef FEATURE_STORE_H
#define FEATURE_STORE_H

#ifndef DISABLE_STL_IO
	#include <fstream>
#endif

#include "cbir/ImageIDArray.h"
#include "cbir/FrameArray.h"
#include "cbir/DescriptorArray.h"

class FeatureStore {
public:
	FeatureStore() {
	}
	~FeatureStore() {
	}
	
	void Append(FeatureStore &aFS) {
		iDescriptorArray.Append(aFS.iDescriptorArray);
		iFrameArray.Append(aFS.iFrameArray);
		iImageIDArray.Append(aFS.iImageIDArray);
	}
	void Append(Descriptor &aDesc, Frame &aFrame, IDType aImageID) {
		iDescriptorArray.Append(aDesc);
		iFrameArray.Append(aFrame);
		iImageIDArray.Append(aImageID);
	}
	void Print() {
		Int len = iDescriptorArray.Size();
		for (Int i = 0; i < len; ++i) {
			DPRINTF(( "%d ", iImageIDArray[i] ));
			iFrameArray[i].Print();
			iDescriptorArray[i].Print();
			DPRINT("\n");
		}
	}

	void Resize(Int aSize) {
		iDescriptorArray.Resize(aSize);
		iFrameArray.Resize(aSize);
		iImageIDArray.Resize(aSize);
	}

	void Clear() {
		Resize(0);
	}

	Int Size() { return iDescriptorArray.Size(); }

	DescriptorArray& GetDescriptorArray()
	{
		return iDescriptorArray;
	}

	FrameArray& GetFrameArray()
	{
		return iFrameArray;
	}
	
	ImageIDArray& GetImageIDArray()
	{
		return iImageIDArray;
	}

	IDType GetImageId(Int aIdx)
	{
		return iImageIDArray[aIdx];	
	}

	void GetUniqueImageIds(vector<IDType> &aFileList)
	{
		iImageIDArray.GetUniqueImageIds(aFileList);
	}
	
	// find all features for a given image id
	void GetImageFeatures(IDType &aImageID, DescriptorArray &aImageDesc)
	{
		Int numDesc = iDescriptorArray.Size();
		for (Int i = 0; i < numDesc; i++)
		{
			if (iImageIDArray[i] == aImageID)
			{
				aImageDesc.Append(iDescriptorArray[i]);
			}
		}
	}

	Frame &GetFrame(Int aIndex) {
		return iFrameArray[aIndex];
	}
	Descriptor &GetDescriptor(Int aIndex) {
		return iDescriptorArray[aIndex];
	}
	IDType GetImageID(Int aIndex) {
		return iImageIDArray[aIndex];
	}

#ifdef DISABLE_STL_IO
	// dummy read/write methods
	void Read(Char *aFile) { return; }
	void Read(Char *aFile, Int aMax) { return; }
	void Write(Char *aFile) { return; }
#else
	// read/write methods
	void Read(Char *aFile) {
		Read(aFile, -1);
	}
	void Read(Char *aFile, Int aMax) {
		Clear();

		Char *bytePtr;

		ifstream file;
		file.open(aFile, ios::binary);
		if (!file.good()) {
			DPRINT("File could not be opened\n");
			return;
		}

		Int size;
		bytePtr = (Char *) &size;
		file.read(bytePtr, sizeof(size));
		if (size <= 0) {
			DPRINT("File is empty or malformed\n");
			return;
		}

		// limit number of features
		if (aMax < size && aMax >= 0) size = aMax;

		for (Int i = 0; i < size; ++i) {
			// read ID
			IDType id;
			bytePtr = (Char *) &id;
			file.read(bytePtr, sizeof(id));

			// read frame
			Frame frame;
			Byte frameDim;
			bytePtr = (Char *) &frameDim;
			file.read(bytePtr, sizeof(frameDim));
			frame.Resize(frameDim);

			for (Int j = 0; j < frameDim; ++j) {
				FrameType val;
				bytePtr = (Char *) &val;
				file.read(bytePtr, sizeof(val));
				frame[j] = val;
			}

			// read desc
			Descriptor descriptor;
			Byte descDim;
			bytePtr = (Char *) &descDim;
			file.read(bytePtr, sizeof(descDim));
			descriptor.Resize(descDim);

			for (Int j = 0; j < descDim; ++j) {
				DescType val = descriptor[j];
				bytePtr = (Char *) &val;
				file.read(bytePtr, sizeof(val));
				descriptor[j] = val;
			}

			Append(descriptor, frame, id);
		}

		file.close();
	}

	void Write(Char *aFile) {
		Char *bytePtr;

		ofstream file;
		file.open(aFile, ios::binary);
		if (!file.good()) {
			DPRINT("File could not be opened");
			return;
		}

		Int size = Size();
		bytePtr = (Char *) &size;
		file.write(bytePtr, sizeof(size));

		for (Int i = 0; i < size; ++i) {
			// save ID
			IDType id = GetImageID(i);
			bytePtr = (Char *) &id;
			file.write(bytePtr, sizeof(id));

			// save frame
			Frame &frame = GetFrame(i);
			Byte frameDim = frame.Size();
			bytePtr = (Char *) &frameDim;
			file.write(bytePtr, sizeof(frameDim));

			for (Int j = 0; j < frameDim; ++j) {
				FrameType val = frame[j];
				bytePtr = (Char *) &val;
				file.write(bytePtr, sizeof(val));
			}

			// save desc
			Descriptor &descriptor = GetDescriptor(i);
			Byte descDim = descriptor.Size();
			bytePtr = (Char *) &descDim;
			file.write(bytePtr, sizeof(descDim));

			for (Int j = 0; j < descDim; ++j) {
				DescType val = descriptor[j];
				bytePtr = (Char *) &val;
				file.write(bytePtr, sizeof(val));
			}
		}

		file.close();
	}
#endif

protected:
	DescriptorArray iDescriptorArray;
	FrameArray iFrameArray;
	ImageIDArray iImageIDArray;
	
};

#endif
