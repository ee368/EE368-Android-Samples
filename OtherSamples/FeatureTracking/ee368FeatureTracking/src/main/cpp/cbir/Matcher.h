#ifndef MATCHER_H
#define MATCHER_H

#include <string>
#include <time.h>

#include "cbir/RifFeatureExtractor.h"
#include "cbir/BruteForce.h"
#include "cbir/L1Distance.h"
#include "cbir/MatchVisualizer.h"
#include "cbir/Ransac.h"

class Matcher {
	// local typedef
	typedef vector< pair<Float, Float> > Polygon;
	typedef Int DbDescType;

public:
	Matcher() {
		Construct();
	}

	void Construct() {
		iDbValid = false;
		iRatioThresh = 0.8;
		iNumDB = 0;
		iNumDbDesc = 0;
		iUniqueDescThresh = 150;

		Char cellConfig[] = "Annuli4Patch35";
		iDbRif.Construct(cellConfig);

		iDbRif.iNumOctaves = 3;
		iDbRif.iScalesPerOctave = 3;

		iQueryPeriod = 1;
		iMinMatches = 4;

		iPlotMatches = false;
	}
	
	void QuantizeDescriptor(Descriptor &aDesc, vector<DbDescType> &aQuantDesc) {
		Int size = aDesc.size();
		aQuantDesc.resize(size);

		for (Int i = 0; i < size; ++i) {
			aQuantDesc[i] = Int(255 * aDesc[i]);
			//aQuantDesc[i] = aDesc[i];
		}
	}

	void BuildDatabase(vector<Char *> &aDbFiles, vector<Char *> &aLabels) {
		// init
		iDbValid = false;
		iNumDB = aDbFiles.size();

		iDatabase.resize(iNumDB);
		iDbDescriptors.resize(iNumDB);
		iLabels.resize(iNumDB);
		iDbImages.resize(iNumDB);

		// extract features;
		iNumDbDesc = 0;
		for (Int i = 0; i < iNumDB; ++i) {
			// load image
			Image<Byte> *image = ImageIO::ReadPGM(aDbFiles[i]);
			iDbImages[i].Copy(*image);
			delete image;

			// extract features at integer octaves
			iDatabase[i].Resize(0);
			iDbRif.ExtractFeatures(iDbImages[i], iDatabase[i], i);

			// make label
			//Char buffer[64];
			//sprintf(buffer, "CD %02d", i);
			//iLabels[i] = buffer;
			iLabels[i] = aLabels[i];

			// flatten descriptors into a 2D array of floats for searching
			iDbDescriptors[i].resize(iDatabase[i].Size());
			for (Int j = 0; j < iDatabase[i].Size(); ++j) {
				Descriptor &desc = iDatabase[i].GetDescriptor(j);
				QuantizeDescriptor(desc, iDbDescriptors[i][j]);
			}

			iNumDbDesc += iDatabase[i].Size();
		}

		RemoveSimilarDescriptors();

		iDbValid = true;
	}

	void Query(Image<Byte> &aImage, FeatureStore &aQueryFS) {
		// get current frames descriptors
		DescriptorArray &queryDescArray = aQueryFS.GetDescriptorArray();
		
		vector< vector<Float> > models(iNumDB);
		vector< vector<Int> > inliers(iNumDB);
		vector< vector< pair<Int,Int> > > matches(iNumDB);

		for (Int i = 0; i < iNumDB; ++i) {
			// search each query descriptor into database
			vector< vector<Int> > nn;		// nearest neighbors
			vector< vector<DistType> > dist;	// distance to nearest neighbors
			ComputeNeighbors(queryDescArray, iDbDescriptors[i], nn, dist);

			// compute matches
			ComputeMatches(nn, dist, matches[i]);
		
			// compute models
			ComputeModel(aQueryFS, iDatabase[i], matches[i], models[i]);

			// outlier removal
			FrameArray &qFrames = aQueryFS.GetFrameArray();
			FrameArray &dbFrames = iDatabase[i].GetFrameArray();
			iRansac.Verify(qFrames, dbFrames, matches[i], inliers[i], models[i]);
		}

		// process results
		if (iPlotMatches) {
			PlotMatches(aImage, aQueryFS, matches, inliers);
		}

		iModels = models;
	}

	void PlotMatches(
			Image<Byte> &aImage,
			FeatureStore &aQueryFeatureStore,
			vector< vector< pair<Int,Int> > > &aMatches,
			vector< vector<Int> > &aInliers) {

		static Int frameNumber = 0;

		printf("frame %03d\n", frameNumber);
		for (Int i = 0; i < iNumDB; ++i) {
			printf("\t%d: %d %d\n", i, aMatches[i].size(), aInliers[i].size());
			
			Char outFile[256];
			sprintf(outFile, "frame%03d_id%02d.ppm", frameNumber, i);
			MatchVisualizer::PlotMatches(iDbImages[i], aImage, 
				iDatabase[i], aQueryFeatureStore,
				aMatches[i], outFile, aInliers[i]);
		}

		++frameNumber;
	}

	// compute nearest neighbors and distances
	void ComputeNeighbors(
			DescriptorArray &aQueryDesc,
			vector< vector<DbDescType> > &aDbDesc,
			vector< vector<Int> > &aNN, 
			vector< vector<DistType> > &aDist) {

		Int k = 2;				// num neighbors
		Int qNumDesc = aQueryDesc.Size();

		aNN.resize(qNumDesc);
		aDist.resize(qNumDesc);

		vector<DbDescType> q;
		for (Int i = 0; i < qNumDesc; ++i) {
			QuantizeDescriptor(aQueryDesc[i], q);

			iBruteForce.FindNN(k, q, aDbDesc, aNN[i], aDist[i]);
		}
	}

	// compute matches given neighbors and distances
	void ComputeMatches(
			vector< vector<Int> > &aNN, 
			vector< vector<DistType> > &aDist, 
			vector< pair<Int,Int> > &aMatches) {

		// init
		aMatches.resize(0);
		Int qNumDesc = aNN.size();

		for (Int i = 0; i < qNumDesc; ++i) {
			Float d1 = aDist[i][0];
			Float d2 = aDist[i][1];

			if (d1 < iRatioThresh * d2) {
				pair<Int, Int> p(i, aNN[i][0]);
				aMatches.push_back(p);
			}
		}
	}

	// compute models given matches
	void ComputeModel(
			FeatureStore &aQuery,
			FeatureStore &aDatabase,
			vector< pair<Int,Int> > &aMatches, 
			vector<Float> &aModel) {

		aModel.resize(0);	// initialize to empty

		// check that we have sufficient matches
		Int numMatches = aMatches.size();
		if (numMatches < iMinMatches) return;

		// compute transform
		AffineSolver solver;
		for (Int j = 0; j < numMatches; ++j) {
			Int qIdx = aMatches[j].first;
			Int dbIdx = aMatches[j].second;

			Frame &qFrame = aQuery.GetFrame(qIdx);
			Frame &dbFrame = aDatabase.GetFrame(dbIdx);

			solver.AddMatch(dbFrame[KX], dbFrame[KY], qFrame[KX], qFrame[KY]);
		}

		solver.ComputeTransform(aModel);
	}
	
	// removes descriptors that are too similar to each other
	void RemoveSimilarDescriptors() {
		iNumDbDesc = 0;

		for (Int i = 0; i < iNumDB; ++i) {
			DescriptorArray &descArray = iDatabase[i].GetDescriptorArray();

			// search each descriptor into database
			vector< vector<Int> > nn;		// nearest neighbors
			vector< vector<DistType> > dist;	// distance to nearest neighbors
			ComputeNeighbors(descArray, iDbDescriptors[i], nn, dist);

			// find unique descriptors
			vector<Int> keepIndices;
			Int numQ = dist.size();
			for (Int j = 0; j < numQ; ++j) {
				// first distance is zero (self), use second distance
				if (dist[j][1] >= iUniqueDescThresh) {	
					keepIndices.push_back(j);
				}
			}

			printf("before: %d\tafter: %d\n", numQ, keepIndices.size());

			// replace database
			vector< vector<DbDescType> > tempDesc = iDbDescriptors[i];
			FeatureStore tempStore = iDatabase[i];

			iDbDescriptors[i].clear();
			iDatabase[i].Clear();

			Int numKeep = keepIndices.size();
			for (Int j = 0; j < numKeep; ++j) {
				Int idx = keepIndices[j];

				Descriptor &desc = tempStore.GetDescriptor(idx);
				Frame &frame = tempStore.GetFrame(idx);
				IDType id = tempStore.GetImageId(idx);

				iDbDescriptors[i].push_back( tempDesc[idx] );
				iDatabase[i].Append(desc, frame, id);
			}

			iNumDbDesc += numKeep;
		}
	}

public:
	// parameters
	Int iNumDB;
	Int iNumDbDesc;
	Float iRatioThresh;
	Int iQueryPeriod;
	Int iMinMatches;
	DistType iUniqueDescThresh;

	Bool iPlotMatches;

	// function objects
	RifFeatureExtractor<Quantize5x5> iDbRif;
	BruteForce<DbDescType, DistType, L1Distance> iBruteForce;
	Ransac iRansac;

	// database
	Bool iDbValid;
	vector<FeatureStore> iDatabase;
	vector< vector< vector<DbDescType> > > iDbDescriptors;
	vector< Image<Byte> > iDbImages;
	vector< vector<Float> > iModels;

	// label data
	vector<string> iLabels;

public:
	const static Int KX = 0;
	const static Int KY = 1;
	const static Int KScl = 2;
	const static Int KOri = 3;
	const static Int KRes = 4;
};

#endif
