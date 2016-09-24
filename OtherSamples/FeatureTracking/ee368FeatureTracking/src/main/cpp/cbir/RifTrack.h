#ifndef RIF_TRACK_H
#define RIF_TRACK_H

#include <math.h>

#include "cbir/stl.h"
#include "cbir/types.h"
#include "cbir/Image.h"
#include "cbir/FastKLDistance.h"
#include "cbir/AffineSolver.h"
#include "cbir/ImageIO.h"
#include "cbir/Quantizers.h"
#include "cbir/RifFeatureExtractor.h"
#include "cbir/FeatureHash.h"

class RifTrack {
public:
#ifdef USE_RIFF_POLAR
	typedef Quantize3x3 QuantizerType;
#else
	typedef Quantize5x5 QuantizerType;
#endif

public:
	RifTrack() {
		Construct();
	}
	void Construct() {
		//============ RIFF Parameters =============//
		// Quantizer must be set as template type above
#ifdef USE_RIFF_POLAR
		Char cellConfig[] = "Polar3x6Patch31Skip";
#else
		//Char cellConfig[] = "Annuli4Patch35";
		Char cellConfig[] = "Annuli4Patch35Skip";
#endif
		iRif.Construct(cellConfig);
		iRif.iBlurImage = false;

		//========== Tracking Parameters ===========//
		iVisualize = false;

#ifdef USE_RIFF_POLAR
		iThresh = 3.0;
		iEarlyTermThresh = 0.5;
#else
		iThresh = 1.0;
		iEarlyTermThresh = 0.5;
#endif
		iMaxFeatures = 100;	// affects speed
		iBinSize = 8;
		iMinTrackedPoints = 3;

		iFrameNumber = 0;

		InitTransform(iTransform);
		InitTransform(iCumulativeTransform);
		iRoll = 0;
		iYaw = 0;
		iPitch = 0;

		// for converting pixels to degrees
		iCameraFOVx = 53;
		iCameraFOVy = 40;

		// initialize feature store buffer
		iBufferSize = 5;
		iFeatureStores.resize(iBufferSize);
		iQueue.Construct(iBufferSize);

		iCurrIndex = 0;
	}

	FeatureStore *GetNextFeatureStore() {
		FeatureStore *pointer = &iFeatureStores[iCurrIndex];
		++iCurrIndex;

		if (iCurrIndex >= iBufferSize) iCurrIndex = 0;
		return pointer;
	}

	Bool TrackFrame(Image<Byte> &aImage, vector< vector<Float> >& aMatchedPoints) {
		Bool valid = false;	// return value

		iWidth = aImage.Width();
		iHeight = aImage.Height();

		// get next feature store
		iCurrFeatureStore = GetNextFeatureStore();
		iQueue.Push(iCurrFeatureStore);

		// extract features from frame
		Float thresh = 0.0;
		iCurrFeatureStore->Resize(0);
		iRif.ExtractFeatures(aImage, *iCurrFeatureStore, iFrameNumber, thresh, iMaxFeatures);
		iMatches.resize(0);
		aMatchedPoints.resize(0);

		AffineSolver affine;
		InitTransform(iTransform);

		if (iFrameNumber > 0) {

			// track features
			Int numFrames = iCurrFeatureStore->Size();
			for (Int i = 0; i < numFrames; ++i) {
				Int minIdx = TrackFeature(i);

				if (minIdx == -1) continue;	// no match

				// save match
				ns::pair<Int,Int> match(i, minIdx);
				iMatches.push_back(match);

				// compute motion vector
				Frame currFrame = iCurrFeatureStore->GetFrame(i);
				Frame prevFrame = iPrevFeatureStore->GetFrame(minIdx);

				affine.AddMatch(prevFrame[KX], prevFrame[KY], currFrame[KX], currFrame[KY]);
				vector<Float> matchXY(4);
				matchXY[0] = prevFrame[KX];
				matchXY[1] = prevFrame[KY];
				matchXY[2] = currFrame[KX];
				matchXY[3] = currFrame[KY];
				aMatchedPoints.push_back(matchXY);
			}
			Int numMatches = iMatches.size();

			// compute transform if we have enough points 
			if (numMatches >= iMinTrackedPoints) {
				affine.ComputeTransform(iTransform);
				valid = true;
			}

			iCumulativeTransform = AffineSolver::AffineMultiply(iCumulativeTransform, iTransform);
			ComputeAngularVelocity();
		}

		// plot Tracking results
		if (iVisualize) Visualize(aImage, valid);

		// create fast location lookup
		iPrevHashTable.Construct(iWidth, iHeight, iBinSize, *iCurrFeatureStore);

		// set pointer to previous frame store
		Int len = iQueue.Size();
		iPrevFeatureStore = iQueue[len-1];

		// increment frame count
		++iFrameNumber;

		return valid;
	}
	
	Bool TrackFrame(Image<Byte> &aImage) {
		vector< vector<Float> > matchedPoints;
		return TrackFrame(aImage, matchedPoints);
	}

	void ComputeAngularVelocity() {
		//==================================
		// find pitch and yaw

		// create a vector in homogeneous coordinates for the central point
		vector<Float> center(3);
		center[0] = iWidth / 2.0;
		center[1] = iHeight / 2.0;
		center[2] = 1;

		// transform the central point
		vector<Float> newCenter = AffineSolver::Transform(iTransform, center);

		// find pitch and yaw
		Float dx = newCenter[0] - center[0];
		Float dy = newCenter[1] - center[1];

		// Camera's field of view
		Float xFOV = iCameraFOVx;	// degrees
		Float yFOV = iCameraFOVy;	// degrees

		Float xDegPerPixel = xFOV / iWidth;
		Float yDegPerPixel = yFOV / iHeight;

		iYaw = dx * xDegPerPixel;
		iPitch = dy * yDegPerPixel;

		//==================================
		// find roll 

		// point to right of center
		Float offset = 50;
		vector<Float> right(3);
		right[0] = iWidth / 2.0 + offset;
		right[1] = iHeight / 2.0;
		right[2] = 1;

		// transform the right point
		vector<Float> newRight = AffineSolver::Transform(iTransform, right);

		dx = newRight[0] - newCenter[0];
		dy = newRight[1] - newCenter[1];

		iRoll = 180/KPi * atan2(dy, dx);
	}

	// make 3x3 identity matrix
	void InitTransform(vector<Float> &aTransform) {
		aTransform.resize(9);

		for (Int i = 0; i < 9; ++i) {
			aTransform[i] = 0;
		}

		aTransform[0] = 1;
		aTransform[4] = 1;
		aTransform[8] = 1;
	}

	Int TrackFeature(Int aIndex) {
		Frame &frame = iCurrFeatureStore->GetFrame(aIndex);
		Descriptor &desc = iCurrFeatureStore->GetDescriptor(aIndex);

		vector<Int> &neighbors = iPrevHashTable.GetNeighbors(frame);
		Int num = neighbors.size();

		// enforce a maximum number of comparisons
		Int maxCompares = 8;
		num = FeatureHash::Min(num, maxCompares);

		Int minIdx = -1;
		Float minDist = ns::numeric_limits<Float>::max();
		for (Int i = 0; i < num; ++i) {
			Int j = neighbors[i];

			Descriptor &prevDesc = iPrevFeatureStore->GetDescriptor(j);

			Float dist = iDist(desc, prevDesc);

			// terminate early if we have a very good match
			if (dist < iEarlyTermThresh) {
				minIdx = j;
				minDist = dist;
				break;
			}

			// otherwise find the best match so far subject to a threshold
			if (dist < minDist && dist < iThresh) {
				minIdx = j;
				minDist = dist;
			}
		}

		return minIdx;
	}

	void Visualize(Image<Byte> &aImage, Bool aValid) {
		static Int counter = 0;

		// draw motion vectors
		vector<Byte> c(3);
		c[0] = 0;
		c[1] = 0;
		c[2] = 255;
		Int numMatches = iMatches.size();
		for (Int i = 0; i < numMatches; ++i) {
			Frame &currFrame = iCurrFeatureStore->GetFrame(iMatches[i].first);
			Frame &prevFrame = iPrevFeatureStore->GetFrame(iMatches[i].second);

			aImage.DrawLine((Int)currFrame[KX], (Int)currFrame[KY], 
					(Int)prevFrame[KX], (Int)prevFrame[KY], c);
		}

		Int w = aImage.Width();
		Int h = aImage.Height();
	
		Float cx = w / 2;
		Float cy = h / 2;
	
		static vector<Float> p0(3);
		static vector<Float> p1(3);
		static vector<Float> p2(3);
	
		// check bounds
		Bool inBounds = true;
		if (p0[1] < 0 || p0[1] >= h) inBounds = false;
		if (p1[1] < 0 || p1[1] >= h) inBounds = false;
		if (p2[1] < 0 || p2[1] >= h) inBounds = false;
		if (p0[0] < 0 || p0[0] >= w) inBounds = false;
		if (p1[0] < 0 || p1[0] >= w) inBounds = false;
		if (p2[0] < 0 || p2[0] >= w) inBounds = false;
	
		// initialize
		if (!counter || !inBounds) {
			p0[0] = cx;
			p0[1] = cy + 20;
			p0[2] = 1;
	
			p1[0] = cx - 20;
			p1[1] = cy - 10;
			p1[2] = 1;
	
			p2[0] = cx + 20;
			p2[1] = cy - 10;
			p2[2] = 1;
		}
	
		// move points
		p0 = AffineSolver::Transform(iTransform, p0);
		p1 = AffineSolver::Transform(iTransform, p1);
		p2 = AffineSolver::Transform(iTransform, p2);
	
		// draw tracked triangle
		c[0] = 255 * !aValid;
		c[1] = 255 * aValid;
		c[2] = 0;
		aImage.DrawLine(p0[0], p0[1], p1[0], p1[1], c);
		aImage.DrawLine(p1[0], p1[1], p2[0], p2[1], c);
		aImage.DrawLine(p2[0], p2[1], p0[0], p0[1], c);
		
		++counter;
	}

public:	// members
	vector< pair<Int,Int> > iMatches;

	FastKLDistance iDist;

	RifFeatureExtractor<QuantizerType> iRif;
	
	FeatureHash iPrevHashTable;
	
	FeatureStore *iCurrFeatureStore;
	FeatureStore *iPrevFeatureStore;
	vector<FeatureStore> iFeatureStores;
	RingBuffer<FeatureStore *> iQueue;
	RingBuffer<Int> iFrameNumbers;
	Int iCurrIndex;
	Int iBufferSize;
	
	Int iFrameNumber;

	Int iMaxFeatures;
	Int iBinSize;
	Int iWidth;
	Int iHeight;
	Int iMinTrackedPoints;

	Float iThresh;
	Float iEarlyTermThresh;

	vector<Float> iTransform;
	vector<Float> iCumulativeTransform;
	Float iRoll;
	Float iYaw;
	Float iPitch;

	Float iCameraFOVx;
	Float iCameraFOVy;

	Bool iVisualize;
private:
	const static Int KX = 0;
	const static Int KY = 1;
	const static Int KScl = 2;
	const static Int KOri = 3;
	const static Int KRes = 4;

	const static Float KPi = 3.14159265358979;
};

#endif
