#ifndef DESCRIPTOR_ARRAY_H
#define DESCRIPTOR_ARRAY_H

#ifndef DISABLE_STL_IO
	#include <fstream>
#endif

using namespace std;

#include "cbir/stl.h"
#include "cbir/Descriptor.h"

class DescriptorArray {
public:
	// constructor and destructors
	
	DescriptorArray() { }
	DescriptorArray(Char* aFile) { Read(aFile); }
	~DescriptorArray() { }

#ifdef DISABLE_STL_IO
	// read/write methods
	void Read(Char *aFile) { return; }
	void Write(Char *aFile) { return; }
	void ReadASCII(Char *aFile) { return; }
	void WriteASCII(Char *aFile) { return; }
#else
	// read/write methods
	void Read(Char *aFile) {
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
		Resize(size);

		for (Int i = 0; i < size; ++i) {
			Byte dim;
			bytePtr = (Char *) &dim;
			file.read(bytePtr, sizeof(dim));
			(*this)[i].Resize(dim);
			if (file.eof()) {
				DPRINT("Premature end of file\n");
				return;
			}

			for (Int j = 0; j < dim; ++j) {
				DescType val;
				bytePtr = (Char *) &val;
				file.read(bytePtr, sizeof(val));
				(*this)[i][j] = val;
				if (file.eof()) {
					DPRINT("Premature end of file\n");
					return;
				}
			}
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
			Byte dim = (*this)[i].Size();
			bytePtr = (Char *) &dim;
			file.write(bytePtr, sizeof(dim));

			for (Int j = 0; j < dim; ++j) {
				DescType val = (*this)[i][j];
				bytePtr = (Char *) &val;
				file.write(bytePtr, sizeof(val));
			}
		}

		file.close();
	}


	void ReadASCII(Char *aFile) {
		ifstream file;
		file.open(aFile);
		if (!file.good()) {
			DPRINT("File could not be opened");
			return;
		}
		Int numDesc;
		file >> numDesc;
		Resize(numDesc);

		Int dim;
		file >> dim;

		for (Int i = 0; i < numDesc; ++i) {
			(*this)[i].Resize(dim);
			for (Int j = 0; j < dim; ++j) {
				file >> (*this)[i][j];
			}
		}

		file.close();
	}

	void WriteASCII(Char *aFile) {
		ofstream file;
		file.open(aFile);
		if (!file.good()) {
			DPRINT("File could not be opened");
			return;
		}

		Int size = Size();
		for (Int i = 0; i < size; ++i) {
			Int dim = (*this)[i].Size();
			for (Int j = 0; j < dim; ++j) {
				file << (Float)(*this)[i][j] << " ";
			}
			file << endl;
		}

		file.close();
	}
#endif

	// accessors
	Descriptor *Get(Int aIndex) {
		return &(iDescriptors[aIndex]);
	}
	void Append(DescriptorArray &aDescriptors) {
		Int size = aDescriptors.Size();
		for (Int i = 0; i < size; ++i) {
			Append(aDescriptors[i]);
		}
	}
	void Append(Descriptor &aDesc) {
		iDescriptors.push_back(aDesc);
	}

	void DeleteLast()
	{
		iDescriptors.pop_back();
	}
	
	void Delete(Int aIndex)
	{
		iDescriptors.erase(iDescriptors.begin()+aIndex);	
	}

	Int Dimension() {
		if (iDescriptors.empty()) return 0;
		return iDescriptors[0].Size();
	}
	Int Size() {
		return iDescriptors.size();
	}
	void Print() {
		Int size = Size();
		for (Int i = 0; i < size; ++i) {
			DPRINTF(("%d : ", i));
			iDescriptors[i].Print();
			DPRINT("\n");
		}
	}

	// set all the elements of the array to zero
	void SetZero() {
		Int size = Size();
		for (Int i = 0; i < size; i++)
		{	
			iDescriptors[i].SetZero();
		}

	}

	// check if two descriptor arrays are the same
	Bool IsEqualTo(DescriptorArray &aOther) {
		Int size = Size();
		if (size != aOther.Size()) {
			return FALSE;
		}
		
		// check if all elements are the same
		for (Int i = 0; i < size; i++) {
			if (!iDescriptors[i].IsEqualTo(aOther[i])) {
				return FALSE;	
			}
		}
		return TRUE;
	}

	// resize
	void Resize(Int aSize) {
		if (aSize >= 0)	iDescriptors.resize(aSize);
	}

	// operators
	Descriptor &operator[] (Int aIndex) {
		return iDescriptors[aIndex];
	}

protected:
	vector< Descriptor > iDescriptors;
	
};

#endif
