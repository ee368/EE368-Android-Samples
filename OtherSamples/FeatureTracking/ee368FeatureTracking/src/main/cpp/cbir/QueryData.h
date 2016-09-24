#ifndef QUERY_DATA_H
#define QUERY_DATA_H

#include "cbir/Tracker.h"
#include "cbir/Matcher.h"

// contains all info needed for communication between query and tracker thread
class QueryData {
public:
	QueryData(	Matcher *aMatcher = NULL,
			Tracker *aTracker = NULL,
			vector<Char *> *aDbFiles = NULL,
			vector<Char *> *aDbLabels = NULL,
			Image<Byte> *aImage = NULL,
			Bool aBuildingDB = false,
			Bool aQueryInProgress = false) {

		iMatcher = aMatcher;
		iTracker = aTracker;
		iImage = aImage;
		iDbFiles = aDbFiles;
		iBuildingDB = aBuildingDB;
		iQueryInProgress = iQueryInProgress;
	}
public:
	Matcher *iMatcher;
	Tracker *iTracker;
	Image<Byte> *iImage;
	Bool iQueryInProgress;
	Bool iBuildingDB;
	FeatureStore iFeatureStore;
	vector<Char *> *iDbFiles;
	vector<Char *> *iDbLabels;
};

#endif
