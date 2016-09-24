#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include "cbir/Image.h"
#include "cbir/FeatureStore.h"

class FeatureExtractor {
public:
	virtual void ExtractFeatures(Image<Byte> &aImage, FeatureStore &aFeatureStore, IDType aImageID)=0;
};

#endif
