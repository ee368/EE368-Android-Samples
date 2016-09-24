#ifndef IMAGE_ID_ARRAY_H
#define IMAGE_ID_ARRAY_H

#include "cbir/stl.h"
#include "cbir/types.h"
#include "cbir/print.h"
#include "cbir/TemplateTypes.h"

class ImageIDArray {
public:
	// constructor and destructors
	ImageIDArray() {
	}
	virtual ~ImageIDArray() {
	}

	// accessors
	virtual IDType Get(Int aIndex) {
		return iIDs[aIndex];
	}
	void Append(ImageIDArray &aIDs) {
		Int size = aIDs.Size();
		for (Int i = 0; i < size; ++i) {
			Append(aIDs[i]);
		}
	}
	virtual void Append(IDType &aID) {
		iIDs.push_back(aID);
	}
	virtual Int Size() {
		return iIDs.size();
	}
	virtual void Clear() { iIDs.clear(); }
	virtual void Print() {
		Int size = Size();
		for (Int i = 0; i < size; ++i) {
			DPRINTF(("%d ",iIDs[i]));
		}
		DPRINT("\n");
	}

	virtual void Resize(Int aSize) {
		if (aSize >= 0) iIDs.resize(aSize);
	}

	virtual Bool IsMember(IDType aID) {
		vector<IDType>::iterator pos = find(iIDs.begin(), iIDs.end(), aID);
		if (pos != iIDs.end()) {
			return TRUE;
		}
		return FALSE;
	}

	virtual Bool AppendUnique(IDType aID)
	{
		Bool unique = !IsMember(aID);
		if (unique) {
			iIDs.push_back(aID);	
		}
		return unique;
	}

	virtual void Unique(ImageIDArray &aFileList) {
		GetUniqueImageIds(aFileList.iIDs);
	}
	virtual void Unique(vector<IDType> &aFileList) {
		GetUniqueImageIds(aFileList);
	}
	virtual void GetUniqueImageIds(vector<IDType> &aFileList)
	{
		Int len = iIDs.size();
		aFileList.clear();
		vector<IDType>::iterator pos;
		for (Int i = 0; i < len; i++)
		{
			pos = find(aFileList.begin(), aFileList.end(), iIDs[i]);
			if (pos == aFileList.end())
			{
				aFileList.push_back(iIDs[i]);	
			}
		}
	}

	// operators
	virtual IDType &operator[] (Int aIndex) {
		return iIDs[aIndex];
	}
protected:
	vector<IDType> iIDs;
};

#endif

