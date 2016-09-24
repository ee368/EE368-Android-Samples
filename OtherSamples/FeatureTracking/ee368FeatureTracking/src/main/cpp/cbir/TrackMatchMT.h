//============================================================================//
//================================= 80 Columns ===============================//

#ifndef TRACK_MATCH_MT_H
#define TRACK_MATCH_MT_H

#include <pthread.h>

#include "cbir/Tracker.h"
#include "cbir/Matcher.h"
#include "cbir/QueryData.h"

class TrackMatchMT {
public:
	// default constructor
	TrackMatchMT() {
		// parameters
		iQueryPeriod = 15;

		// priorities in range [1 99]
		iTrackPriority = 90;
		iBuildPriority = 10;
		iQueryPriority = 90;

		// init
		iFrame = 0;
		iCumModel = AffineSolver::Eye();

		iQueryData.iMatcher = &iMatcher;
		iQueryData.iTracker = &iTracker;
		iQueryData.iDbFiles = &iDbFiles;
		iQueryData.iDbLabels = &iDbLabels;
	
		// database images
		Int numDB = 2;
		Char *files[] = {(Char *)"DB-50sb/01.pgm",
				 (Char *)"DB-50sb/02.pgm",
				 (Char *)"DB-50sb/03.pgm",
				 (Char *)"DB-50sb/04.pgm",
				 (Char *)"DB-50sb/05.pgm",
				 (Char *)"DB-50sb/06.pgm",
				 (Char *)"DB-50sb/07.pgm",
				 (Char *)"DB-50sb/08.pgm",
				 (Char *)"DB-50sb/09.pgm",
				 (Char *)"DB-50sb/10.pgm"};
		for (Int i = 0; i < numDB; ++i) iDbFiles.push_back(files[i]);

		Char *labels[] = {(Char *)"Keys",
				 (Char *)"Murray",
				 (Char *)"Beatles",
				 (Char *)"Caillat",
				 (Char *)"Cat Power",
				 (Char *)"Fogelberg",
				 (Char *)"Brooks",
				 (Char *)"White",
				 (Char *)"Springsteen",
				 (Char *)"Horse"};
		for (Int i = 0; i < numDB; ++i) iDbLabels.push_back(labels[i]);
	}

	//==========================================
	// thread functions

	// performs querying in separate thread
	static void *QueryDatabaseThread(void *aArg) {
		QueryData *data = (QueryData *)aArg;

		if (data->iQueryInProgress) return NULL;
		data->iQueryInProgress = true;
	
		data->iMatcher->Query(*data->iImage, data->iFeatureStore);
		data->iQueryInProgress = false;
	
		return NULL;
	}

	// create database for periodic matching
	static void *BuildDatabaseThread(void *aArg) {
		QueryData *data = (QueryData *)aArg;
		
		data->iMatcher->BuildDatabase(*data->iDbFiles, *data->iDbLabels);
		data->iBuildingDB = false;

		return NULL;
	}

	//==========================================
	// Primary function

	void TrackFrame(Image<Byte> &aImage) {
		// frame counter
		++iFrame;
	
		// set this thread to top priority
		struct sched_param param;
		param.sched_priority = iTrackPriority;
		pthread_setschedparam(pthread_self(), SCHED_RR, &param);

		// spawn thread to build database
		if (!iMatcher.iDbValid && !iQueryData.iBuildingDB) {
			iQueryData.iBuildingDB = true;

			// thread schedule parameters
			struct sched_param param;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			param.sched_priority = iBuildPriority;
			pthread_attr_setschedparam(&attr, &param);
//			pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	
			void *arg = (void *)&iQueryData;
			pthread_t databaseThreadID;
			pthread_create(&databaseThreadID, &attr, 
					BuildDatabaseThread, arg);
	
			return;	// nothing to track
		}

		// are we still building the database
		if (iQueryData.iBuildingDB) {
			return;	// nothing to track
		}

		// query has finished
		if (!iQueryData.iQueryInProgress && iMatcher.iDbValid) {
			// update models
			vector< vector<Float> > models = iMatcher.iModels;

			Int numModels = models.size();
			for (Int j = 0; j < numModels; ++j) {
				if (models[j].size() == 0) continue;
				models[j] = AffineSolver::
					AffineMultiply(iCumModel, models[j]);
			}

			iTracker.UpdatePolygons(models, iMatcher.iDbImages, 
					iMatcher.iLabels);
		}

		// track
		Bool validModel = iTracker.TrackFrame(aImage);
		if (!validModel) Reset();
		iQueryData.iImage = &aImage;
		iTracker.DrawPolygons(aImage);
	
		// update cumulative model
		if (iQueryData.iQueryInProgress) {
			vector<Float> &model = iTracker.iTracker.iTransform;
			iCumModel = AffineSolver::
				AffineMultiply(model, iCumModel);
		}

		// periodic matching 
		if (iFrame % iQueryPeriod == 0 && !iQueryData.iQueryInProgress) {

			// prepare model so that matching isn't stale
			iCumModel = AffineSolver::Eye();

			// copy feature store so it doesnt get clobbered
			iQueryData.iFeatureStore = 
				*iTracker.iTracker.iCurrFeatureStore;

			// thread schedule parameters
			struct sched_param param;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			param.sched_priority = iQueryPriority;
			pthread_attr_setschedparam(&attr, &param);
//			pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	
			// create database thread for periodic matching
			void *arg = (void *)&iQueryData;
			pthread_t databaseThreadID;
			pthread_create(&databaseThreadID, &attr, 
					QueryDatabaseThread, arg);
		}

		// indicate to user when we are querying
		if (iQueryData.iQueryInProgress) {
			vector<Byte> color(1); 
			color[0] = 255;

			aImage.DrawBox(310, 230, 10, 10, 0, color);
		}

	}

	void Reset() {
		iTracker.Reset();
	}

public:
	//==========================================
	// Member variables 

	Int iQueryPeriod;
	Int iFrame;

	vector<Char *> iDbFiles;
	vector<Char *> iDbLabels;
	vector<Float> iCumModel;

	Matcher iMatcher;
	Tracker iTracker;
	QueryData iQueryData;

	Int iQueryPriority;
	Int iBuildPriority;
	Int iTrackPriority;

};

#endif
