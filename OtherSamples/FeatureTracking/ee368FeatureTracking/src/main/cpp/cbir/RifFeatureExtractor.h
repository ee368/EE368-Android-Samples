#ifndef RIF_FEATURE_EXTRACTOR_H
#define RIF_FEATURE_EXTRACTOR_H

#include "cbir/FeatureExtractor.h"
#include "cbir/Image.h"
#include "cbir/FeatureStore.h"
#include "cbir/FAST.h"
#include "cbir/Fixed.h"
#include "cbir/ImageIO.h"
#include "cbir/MipMap.h"
#include "cbir/CellMap.h"

#include <time.h>
#include <math.h>

template <class Quantizer>
class RifFeatureExtractor : public FeatureExtractor {
public:
	class PatchData {
	public:
		PatchData() {
			xStart = 0;
			xStop = 0;
			yStart = 0;
			yStop = 0;
			xCenter = 0;
			yCenter = 0;
			width = 0;
			height = 0;
			octave = 0;
			outOfBounds = false;
		}
	public:
		Int xStart;
		Int xStop;
		Int yStart;
		Int yStop;
		Int height;
		Int width;
		Int xCenter;
		Int yCenter;
		Int octave;
		Bool outOfBounds;
	};
	class GradientData {
	public:
		GradientData() {
			x0 = 0;
			x1 = 0;
			y0 = 0;
			y1 = 0;
			baselineIdx = 0;
		}
	public:
		Int x0, x1, y0, y1;
		Int baselineIdx;
	};
public:
	RifFeatureExtractor() {
		Construct();
	}
	RifFeatureExtractor(Char *aCellConfig) {
		Construct(aCellConfig);
	}
	void Construct() {
		Construct( (Char *)"Polar3x6Patch31Skip" );	// default
	}
	void Construct(Char *aCellConfig) {
		// scale space
		iScalesPerOctave = 1;
		iNumOctaves = 1;

		iBlurImage = false;

		// cell config
		iCells.Construct(aCellConfig);
		iPatchSize = iCells.iPatchSize;

		iLog2PatchStride = (Int) ceil(log(iPatchSize) / log(2));

		PrecomputeGradientData();

		//DrawCells();
	}
	void DrawCells() {
		Image<Float> image(iPatchSize, iPatchSize, 1);

		Int counter = 0;
		for (Int i = 0; i < iCells.Size(); ++i) {

			Int numPixels = iCells[i].size();
			for (Int j = 0; j < numPixels; ++j) {
				Int x = iCells[i][j].first;
				Int y = iCells[i][j].second;

				image(x,y) = i+1;
				counter++;
			}
		}
		printf("num pixels = %d\n", counter);

		ImageIO::WritePGM((Char*)"cells.pgm", &image);
	}

	void ComputeBaselineTable(Float aStdevInv = 1) {
		iBaselineTable.resize(4);
		iBaselineTable[0] = TFixed::FromReal(aStdevInv * 0.5);
		iBaselineTable[1] = TFixed::FromReal(aStdevInv);
		iBaselineTable[2] = TFixed::FromReal(aStdevInv * 0.353553390593274);
		iBaselineTable[3] = TFixed::FromReal(aStdevInv * 0.707106781186547);
	}
	void ComputeGradDir() {
		iGradDir.Construct(iPatchSize, iPatchSize, 1);

		Float center = (iPatchSize-1) / 2.0;
		Float rad2deg = 180 / KPi;

		Int numDir = 8;
		Float delta = 360.0 / numDir;

		for (Int i = 0; i < iPatchSize; ++i) {
			for (Int j = 0; j < iPatchSize; ++j) {
				Float x = i - center;
				Float y = j - center;

				Float theta = rad2deg * atan2(y, x);
				
				Int bin = (Int) Round(theta / delta);
				bin = Mod(bin, numDir);

				iGradDir(i, j) = bin;
			}
		}
	}

	void ExtractFeatures(Image<Byte> &aImage, FeatureStore &aFeatureStore, IDType aImageID) {
		ExtractFeatures(aImage, aFeatureStore, aImageID, 0, -1);
	}
	void ExtractFeatures(Image<Byte> &aImage, FeatureStore &aFeatureStore, IDType aImageID, Float aThreshold, Int aMaxFeatures = -1) {
		FrameArray frames;

		// pointer to the input image, this may be changed
		Image<Byte> *image = &aImage;

		// this is a small blur which may help robustness of FAST interest points
		Image<Byte> blurImage;
		if (iBlurImage) {
			blurImage.Copy(aImage);
			blurImage.BlurPow2(4);
			image = &blurImage;
		}

		// extract at single scale
		if (iNumOctaves == 1) {
			DetectInterestPoints(*image, frames, aThreshold, aMaxFeatures);

			// always extract features from original image
			ExtractFeatures(frames, aImage, aFeatureStore, aImageID, aThreshold, aMaxFeatures);

		// extract at multiple scales
		} else if (iNumOctaves > 1) {
			Int prevStoreSize = aFeatureStore.Size();

			for (Int j = 0; j < iScalesPerOctave; ++j) {
				Image<Byte> *baseImage = image;

				Float exponent = Float(j) / iScalesPerOctave;
				if (j > 0) {
					Float scaleFactor = pow(2.0, -exponent);

					Int newWidth  = scaleFactor * image->Width();
					Int newHeight = scaleFactor * image->Height();
	
					baseImage = image->Resize(newWidth, newHeight);
				}

				// create image pyramid
				iMipMap.Construct(*baseImage);

				// loop over scales
				for (Int i = 0; i < iNumOctaves; ++i) {
					Int numLevels = iMipMap.iImages.size();
					if (i >= numLevels) break;
					Float scale = exponent + i;

					Image<Byte> &level = iMipMap.iImages[i];
					DetectInterestPoints(level, frames, aThreshold, aMaxFeatures, scale);
					ExtractFeatures(frames, level, aFeatureStore, aImageID, aThreshold, aMaxFeatures);
				}

				if (j > 0) delete baseImage;
			}

			// change (x,y) positions to match those in the full image
			Float prevScale = 0;
			Float scaleFactor = 1;
			for (Int i = prevStoreSize; i < aFeatureStore.Size(); ++i) {
				Frame &frame = aFeatureStore.GetFrame(i);

				if (prevScale != frame[KScl]) {
					scaleFactor = pow(2.0, frame[KScl]);
					prevScale = frame[KScl];
				}

				frame[KX] *= scaleFactor;
				frame[KY] *= scaleFactor;
			}
		}

	}
		
	void ExtractFeatures(FrameArray &aFrames, Image<Byte> &aImage, FeatureStore &aFeatureStore, IDType aImageID, Float aThreshold, Int aMaxFeatures = -1) {
		Int numFrames = aFrames.Size();
		
		// compute interest points
		iImage = &aImage;

		// loop over interest points
		Int numCells = iCells.Size();
		Int numBins = iQuantizer.iNumBins;
		Int descDim = numCells * numBins;
		Descriptor desc(descDim);
		vector<Int> pdf(numBins);

		for (Int k = 0; k < numFrames; ++k) {

			// find patch extent
			PatchData patchData;
			ComputePatchData(aFrames[k], patchData);
			
			// do not worry about points that are at edge of image
			if (patchData.outOfBounds) continue;

			// compute mean and variance for normalization
			Float mean = 0;
			Float variance = 1;
			ComputeVariance(patchData, mean, variance);
			Float stdevInv = 1.0 / sqrt(variance);
			ComputeBaselineTable(stdevInv);

			// loop over cells, compute gradients and build histogram
			Int descIdx = 0;
			for (Int i = 0; i < numCells; ++i) {

				// initialize with prior
				for (Int j = 0; j < numBins; ++j) pdf[j] = 1;

				// iCells[i] is a vector of pairs
				Int numPixels = iCells[i].size();
				for (Int j = 0; j < numPixels; ++j) {
					Int x = iCells[i][j].first;
					Int y = iCells[i][j].second;

					Int idx = (x << iLog2PatchStride) + y;

					TFixed dr = ComputeGradient(patchData, iGradDataR[idx]);
					TFixed dt = ComputeGradient(patchData, iGradDataT[idx]);

					Int idxQ = iQuantizer(dr, dt);
					++pdf[idxQ];
				}

				// normalize pdf
				Int sum = 0;
				for (Int j = 0; j < numBins; ++j) sum += pdf[j];
				Float sumInv = 1.0 / sum;
				
				for (Int j = 0; j < numBins; ++j) {
					desc[descIdx] = pdf[j] * sumInv;
					++descIdx;
				}
			}

#ifndef USE_RIFF_POLAR
			// new order speeds up distance computation
			// sorted by variance
			ReOrderDescriptor(desc);
#endif

			// store feature
			aFeatureStore.Append(desc, aFrames[k], aImageID);
		}
	}

	inline void ReOrderDescriptor(Descriptor &aDesc) {
		static Int idx[] = {88,63,38,13,5,21,1,25,23,3,8,18,43,68,33,93,58,83,26,30,50,46,37,39,4,2,28,24,62,22,64,87,51,89,55,14,12,48,71,75,76,80,53,100,96,78,73,98,15,11,9,17,19,7,42,44,32,34,36,40,67,10,69,57,59,16,6,20,61,27,92,29,65,94,82,84,49,47,86,90,52,41,35,31,45,54,74,72,77,99,79,97,66,60,70,56,85,91,81,95};

		Int dim = aDesc.size();
		Descriptor tempDesc = aDesc;
		for (Int i = 0; i < dim; ++i) {
			Int j = idx[i] - 1;	// -1 for matlab to C indices
			aDesc[i] = tempDesc[j];
		}
	}
	
	inline TFixed ComputeGradient(PatchData &pd, GradientData &gd) {
		Image<Byte> &image = *iImage;

		// compute finite difference
		TFixed grad;

		grad = image(gd.x0 + pd.xStart, gd.y0 + pd.yStart);
		grad -= image(gd.x1 + pd.xStart, gd.y1 + pd.yStart);
		
		// includes intensity normalization
		grad *= iBaselineTable[gd.baselineIdx];

		return grad;
	}

	// single pass variance
	void ComputeVariance(PatchData &aPatchData, Float &aMean, Float &aVariance) {
		Image<Byte> &image = *iImage;

		// var = E[(x-E[x])^2] 
		//     = E[x^2] - E[x]^2

		// compute the mean
		long long mean = 0;
		long long meanSquare = 0;
		Int counter = 0;

		Int numCells = iCells.Size();
		for (Int i = 0; i < numCells; ++i) {
			Int numPixels = iCells[i].size();
			for (Int j = 0; j < numPixels; ++j) {
				Int x = aPatchData.xStart + iCells[i][j].first;
				Int y = aPatchData.yStart + iCells[i][j].second;

				Byte val = image(x, y);

				meanSquare += val*val;
				mean += val;
				++counter;
			}
		}

		aMean = mean / Float(counter);
		aVariance = meanSquare / Float(counter) - aMean*aMean;
	}

	void PrecomputeGradientData() {
		Int patchStride = 1 << iLog2PatchStride;

		iGradDataT.resize(patchStride * patchStride);
		iGradDataR.resize(patchStride * patchStride);

		ComputeGradDir();

		for (Int i = 0; i < iPatchSize; ++i) {
			for (Int j = 0; j < iPatchSize; ++j) {
				Int binR = iGradDir(i, j);
				Int binT = Mod(binR+2, 8);

				Int idx = i*patchStride + j;

				GradientData &gdR = iGradDataR[idx];
				GradientData &gdT = iGradDataT[idx];

				ComputeGradientData(binR, i, j, gdR);
				ComputeGradientData(binT, i, j, gdT);
			}
		}
	}
	void ComputeGradientData(Int aBin, Int aI, Int aJ, GradientData &aGradData) {
		Int xPlus = 0;
		Int yPlus = 0;
		Int baselineIdx = 0;

		switch (aBin) {
		case 7:	// up right
			xPlus = 1;
			yPlus = -1;
			baselineIdx = 2;
			break;
		case 6: // up
			xPlus = 0;
			yPlus = -1;
			baselineIdx = 0;
			break;
		case 5: // up left
			xPlus = -1;
			yPlus = -1;
			baselineIdx = 2;
			break;
		case 4:	// left
			xPlus = -1;
			yPlus = 0;
			baselineIdx = 0;
			break;
		case 3:	// left down
			xPlus = -1;
			yPlus = 1;
			baselineIdx = 2;
			break;
		case 2:	// down
			xPlus = 0;
			yPlus = 1;
			baselineIdx = 0;
			break;
		case 1: // down right
			xPlus = 1;
			yPlus = 1;
			baselineIdx = 2;
			break;
		case 0:	// right
			xPlus = 1;
			yPlus = 0;
			baselineIdx = 0;
			break;
		}

		Int x0 = aI + xPlus;
		Int y0 = aJ + yPlus;
		Int x1 = aI - xPlus;
		Int y1 = aJ - yPlus;

		// check bounds and adjust baseline accordingly
		// revert to assymetric gradient
		Bool outOfBounds = false;

		if (x0 < 0) { x0 = 0; outOfBounds = true; }
		if (y0 < 0) { y0 = 0; outOfBounds = true; }
		if (x1 < 0) { x1 = 0; outOfBounds = true; }
		if (y1 < 0) { y1 = 0; outOfBounds = true; }

		if (x0 >= iPatchSize) { x0 = iPatchSize-1; outOfBounds = true; }
		if (y0 >= iPatchSize) { y0 = iPatchSize-1; outOfBounds = true; }
		if (x1 >= iPatchSize) { x1 = iPatchSize-1; outOfBounds = true; }
		if (y1 >= iPatchSize) { y1 = iPatchSize-1; outOfBounds = true; }

		if (outOfBounds) {
			++baselineIdx;
		}

		aGradData.x0 = x0;
		aGradData.y0 = y0;
		aGradData.x1 = x1;
		aGradData.y1 = y1;
		aGradData.baselineIdx = baselineIdx;
	}
	
	void ComputePatchData(Frame &aFrame, PatchData &aData) {
		aData.octave = Int(aFrame[KScl]);

		Image<Byte> &image = *iImage;
		Int width = image.Width();
		Int height = image.Height();

		Int halfPatchSize = iPatchSize >> 1;

		aData.xCenter = Int(aFrame[KX]) >> aData.octave;
		aData.yCenter = Int(aFrame[KY]) >> aData.octave;

		aData.xStart = aData.xCenter - halfPatchSize;
		aData.yStart = aData.yCenter - halfPatchSize;

		// check for bounds
		if (aData.xStart < 0) { aData.xStart = 0; aData.outOfBounds = true; return; }
		if (aData.xStart >= width) { aData.xStart = width-1; aData.outOfBounds = true; return; }

		if (aData.yStart < 0) { aData.yStart = 0; aData.outOfBounds = true; return; }
		if (aData.yStart >= height) { aData.yStart = height-1; aData.outOfBounds = true; return; }

		aData.xStop = aData.xCenter + halfPatchSize;
		aData.yStop = aData.yCenter + halfPatchSize;

		if (aData.xStop < 0) { aData.xStop = 0; aData.outOfBounds = true; return; }
		if (aData.xStop >= width) { aData.xStop = width-1; aData.outOfBounds = true; return; }

		if (aData.yStop < 0) { aData.yStop = 0; aData.outOfBounds = true; return; }
		if (aData.yStop >= height) { aData.yStop = height-1; aData.outOfBounds = true; return; }

		aData.width = aData.xStop - aData.xStart;
		aData.height = aData.yStop - aData.yStart;
	}

	virtual void DetectInterestPoints(Image<Byte> &aImage, FrameArray &aFrames) {
		DetectInterestPoints(aImage, aFrames, 0, -1);
	}
	virtual void DetectInterestPoints(Image<Byte> &aImage, FrameArray &aFrames, Float aThresh, Int aMaxFeatures, Float aScale = 0) {
		FAST fast;

		// run FAST
		xy* corners;
		xy* nonmax;
		Int* scores;
		Int numCorners;
		Int nonMaxCorners;

		Image<Byte> &image = aImage;
		Int w = image.Width();
		Int h = image.Height();
		Byte *im = image.PixelPointer(0,0);
		
		corners = fast.fast9_detect(im, w, h, w, iThresh, &numCorners);
		scores = fast.fast9_score(im, w, corners, numCorners, iThresh);
		nonmax = fast.nonmax_suppression(corners, scores, numCorners, &nonMaxCorners);

		// copy results
		Int frameSize = 5;
		Frame frame(frameSize);
		for (Int i = 0; i < nonMaxCorners; ++i) {
			frame[KX] = nonmax[i].x;
			frame[KY] = nonmax[i].y;
			frame[KScl] = aScale;
			frame[KOri] = 0;
			frame[KRes] = -scores[i];

			if (scores[i] < aThresh) continue;

			aFrames.Append(frame);
		}

		free(corners);
		free(scores);
		free(nonmax);

		if (aMaxFeatures >= 0 && aFrames.Size() > aMaxFeatures) {
			aFrames.Sort(KRes);
			aFrames.Resize(aMaxFeatures);
		}
	}

	template<class T>
	inline T Floor(T x) {
		if (x < 0) return (T) Int(x-1);
		return (T) Int(x);

	}
	template<class T>
	inline T Round(T x) {
		return (T) Floor(x + 0.5);
	}
	template<class T>
	inline T Max(T a, T b) {
		if (a < b) return b;
		return a;
	}
	template<class T>
	inline T Min(T a, T b) {
		if (a < b) return a;
		return b;
	}
	template<class T>
	inline T Mod(T x, T m) {
		while (x < 0) x += m;
		while (x >= m) x -= m;
		return x;
	}

public:
	Bool iBlurImage;
	CellMap iCells;
	Int iNumOctaves;
	Int iScalesPerOctave;

private:
	Int iPatchSize;
	Int iLog2PatchStride;

	Image<Byte> *iImage;

	Image<Byte> iGradDir;
	vector<GradientData> iGradDataT;
	vector<GradientData> iGradDataR;
	vector<TFixed> iBaselineTable;

	Quantizer iQuantizer;

	MipMap<Byte> iMipMap;
private:
	const static Int iThresh = 30;

	const static Int KX = 0;
	const static Int KY = 1;
	const static Int KScl = 2;
	const static Int KOri = 3;
	const static Int KRes = 4;

	const static Float KPi = 3.14159265358979;
};

#endif



