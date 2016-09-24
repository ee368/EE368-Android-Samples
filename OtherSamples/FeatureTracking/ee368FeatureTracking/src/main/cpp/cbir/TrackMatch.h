#ifndef TRACK_MATCH_H
#define TRACK_MATCH_H

#include <string>

#include "cbir/RifTrack.h"
#include "cbir/BruteForce.h"
#include "cbir/L1Distance.h"
#include "cbir/MatchVisualizer.h"
#include "cbir/Ransac.h"
#include "cbir/Font.h"

class TrackMatch {
	// local typedef
	typedef vector< pair<Float, Float> > Polygon;
	typedef Int DbDescType;

public:
	TrackMatch() {
		Construct();
	}

	void Construct() {
		iUniqueDescThresh = 150;
		iRatioThresh = 0.8;
		iNumDB = 0;
		iNumDbDesc = 0;

		Char cellConfig[] = "Annuli4Patch35";
		iDbRif.Construct(cellConfig);

		iDbRif.iNumOctaves = 3;
		iDbRif.iScalesPerOctave = 3;

		iQueryPeriod = 1;
		iMinMatches = 4;

		iPlotMatches = true;
		iVisualize = false;
	}
	
	Bool TrackFrame(Image<Byte> &aImage) {
		Bool tracked = iTracker.TrackFrame(aImage);

		if (iTracker.iFrameNumber % iQueryPeriod == 0) {
			Query(aImage);
		}

		TrackPolygons();

		if (iVisualize) Visualize(aImage);

		return tracked;
	}

	void QuantizeDescriptor(Descriptor &aDesc, vector<DbDescType> &aQuantDesc) {
		Int size = aDesc.size();
		aQuantDesc.resize(size);

		for (Int i = 0; i < size; ++i) {
			aQuantDesc[i] = Int(255 * aDesc[i]);
			//aQuantDesc[i] = aDesc[i];
		}
	}

	void BuildDatabase(vector<Char *> &aDbFiles) {
		// init
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
			Char buffer[64];
			sprintf(buffer, "CD %02d", i);
			iLabels[i] = buffer;

			// flatten descriptors into a 2D array of floats for searching
			iDbDescriptors[i].resize(iDatabase[i].Size());
			for (Int j = 0; j < iDatabase[i].Size(); ++j) {
				Descriptor &desc = iDatabase[i].GetDescriptor(j);
				QuantizeDescriptor(desc, iDbDescriptors[i][j]);
			}

			iNumDbDesc += iDatabase[i].Size();
		}

		RemoveSimilarDescriptors();
	}

	void Query(Image<Byte> &aImage) {
		// get current frames descriptors
		DescriptorArray &qDescArray = iTracker.iCurrFeatureStore->GetDescriptorArray();
		
		vector< vector<Float> > models(iNumDB);
		vector< vector<Int> > inliers(iNumDB);
		vector< vector< pair<Int,Int> > > matches(iNumDB);

		for (Int i = 0; i < iNumDB; ++i) {

			// search each query descriptor into database
			vector< vector<Int> > nn;		// nearest neighbors
			vector< vector<DistType> > dist;	// distance to nearest neighbors
			ComputeNeighbors(qDescArray, iDbDescriptors[i], nn, dist);

			// compute matches
			ComputeMatches(nn, dist, matches[i]);
		
			// compute models
			ComputeModel(*iTracker.iCurrFeatureStore, iDatabase[i], matches[i], models[i]);

			// outlier removal
			FrameArray &qFrames = iTracker.iCurrFeatureStore->GetFrameArray();
			FrameArray &dbFrames = iDatabase[i].GetFrameArray();
			iRansac.Verify(qFrames, dbFrames, matches[i], inliers[i], models[i]);
		}

		// update polygons
		UpdatePolygons(models);

		// process results
		if (iPlotMatches) {
			PlotMatches(aImage, *iTracker.iCurrFeatureStore, matches, inliers);
		}
	}

	void Visualize(Image<Byte> &aImage) {
		// construct file name
		Char outFile[256];
		sprintf(outFile, "output%03d.ppm", iTracker.iFrameNumber);

		// copy image to output buffer
		Int w = aImage.Width();
		Int h = aImage.Height();
		Image<Byte> img;
		img.Construct(w, h, 3);
		for (Int x = 0; x < w; ++x) {
			for (Int y = 0; y < h; ++y) {
				img(x, y, 0) = aImage(x, y);
				img(x, y, 1) = aImage(x, y);
				img(x, y, 2) = aImage(x, y);
			}
		}	

		// make colors
		vector<Byte> blue(3);
		blue[0] = 0;
		blue[1] = 0;
		blue[2] = 255;

		// draw outlines
		Int numPoly = iPolyIDs.size();
		for (Int i = 0; i < numPoly; ++i) {
			Int numCorners = iPolygons[i].size();
			for (Int j = 0; j < numCorners; ++j) {
				Int idx0 = j;
				Int idx1 = (j + 1) % numCorners;

				Int x0 = (Int)iPolygons[i][idx0].first;
				Int y0 = (Int)iPolygons[i][idx0].second;

				Int x1 = (Int)iPolygons[i][idx1].first;
				Int y1 = (Int)iPolygons[i][idx1].second;

				img.DrawLine(x0, y0, x1, y1, blue);
			}
		}

		// draw labels
		for (Int i = 0; i < numPoly; ++i) {
			// find poly center
			Float xMean = 0;
			Float yMean = 0;
			Int numCorners = iPolygons[i].size();
			for (Int j = 0; j < numCorners; ++j) {
				xMean += iPolygons[i][j].first;
				yMean += iPolygons[i][j].second;
			}
			xMean /= numCorners;
			yMean /= numCorners;

			Int fw = Glyphs::iFontWidth;
			Int fh = Glyphs::iFontHeight;

			Int labelLen = iLabels[iPolyIDs[i]].size();

			Int xOffset = fw * labelLen / 2;
			Int yOffset = fh / 2;

			Char *str = (Char *)iLabels[iPolyIDs[i]].c_str();
			img.Text(xMean-xOffset, yMean-yOffset, str, blue);
		}

		// save image
		ImageIO::WritePPM(outFile, img);
	}

	void TrackPolygons() {
		Int num = iPolyIDs.size();

		for (Int i = 0; i < num; ++i) {
			Int numCorners = iPolygons[i].size();
			for (Int j = 0; j < numCorners; ++j) {
				vector<Float> x(3);
				x[0] = iPolygons[i][j].first;
				x[1] = iPolygons[i][j].second;
				x[2] = 1;

				vector<Float> y = AffineSolver::Transform(iTracker.iTransform, x);

				iPolygons[i][j].first = y[0];
				iPolygons[i][j].second = y[1];
			}
		}
	}

	void UpdatePolygons(vector< vector<Float> > &aModels) {
		for (Int i = 0; i < iNumDB; ++i) {
			// see if we are already tracking
			Bool isTracked = false;
			Int numPoly = iPolyIDs.size();
			for (Int j = 0; j < numPoly; ++j) {
				if (iPolyIDs[j] == i) isTracked = true;
			}

			Bool hasModel = aModels[i].size();

			if (hasModel && !isTracked) {
				// create database polygon
				Int w = iDbImages[i].Width();
				Int h = iDbImages[i].Height();

				Int numCorners = 4;
				Polygon poly(numCorners);

				poly[0].first = 0;
				poly[0].second = 0;

				poly[1].first = w-1;
				poly[1].second = 0;

				poly[2].first = w-1;
				poly[2].second = h-1;

				poly[3].first = 0;
				poly[3].second = h-1;

				// map db poly into query
				for (Int j = 0; j < numCorners; ++j) {
					vector<Float> x(3);
					x[0] = poly[j].first;
					x[1] = poly[j].second;
					x[2] = 1;

					vector<Float> y = AffineSolver::Transform(aModels[i], x);

					poly[j].first = y[0];
					poly[j].second = y[1];
				}

				// store polygon
				iPolyIDs.push_back(i);
				iPolygons.push_back(poly);
			}
		}
	}

	void PlotMatches(
			Image<Byte> &aImage,
			FeatureStore &aQueryFeatureStore,
			vector< vector< pair<Int,Int> > > &aMatches,
			vector< vector<Int> > &aInliers) {

		printf("frame %03d\n", iTracker.iFrameNumber);
		for (Int i = 0; i < iNumDB; ++i) {
			printf("\t%d: %d %d\n", i, aMatches[i].size(), aInliers[i].size());
			
			Char outFile[256];
			sprintf(outFile, "frame%03d_id%02d.ppm", iTracker.iFrameNumber, i);
			MatchVisualizer::PlotMatches(iDbImages[i], aImage, 
				iDatabase[i], aQueryFeatureStore,
				aMatches[i], outFile, aInliers[i]);
		}
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
	Bool iVisualize;

	// function objects
	RifTrack iTracker;
	RifFeatureExtractor<Quantize5x5> iDbRif;
	BruteForce<DbDescType, DistType, L1Distance> iBruteForce;
	Ransac iRansac;

	// database
	vector<FeatureStore> iDatabase;
	vector< vector< vector<DbDescType> > > iDbDescriptors;
	vector< Image<Byte> > iDbImages;

	// label data
	vector<Polygon> iPolygons;
	vector<IDType> iPolyIDs;
	vector<string> iLabels;

public:
	const static Int KX = 0;
	const static Int KY = 1;
	const static Int KScl = 2;
	const static Int KOri = 3;
	const static Int KRes = 4;
};

#endif
