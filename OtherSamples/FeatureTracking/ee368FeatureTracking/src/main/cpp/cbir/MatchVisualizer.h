#ifndef MATCH_VISUALIZER_H
#define MATCH_VISUALIZER_H

#include "cbir/Image.h"
#include "cbir/ImageIO.h"

class MatchVisualizer {
public:
	static void PlotMatches(
		Char *aDbFile, 
		Char *aQueryFile, 
		FeatureStore &aDbFeatures, 
		FeatureStore &aQueryFeatures,
		vector< pair<Int,Int> > &aMatches, 
		Char *aOutFile) 
	{
		// load images
		Image<Byte> *qImg = ImageIO::ReadPGM(aQueryFile);
		Image<Byte> *dbImg = ImageIO::ReadPGM(aDbFile);

		PlotMatches(*dbImg, *qImg, aDbFeatures, aQueryFeatures, aMatches, aOutFile);

		delete qImg;
		delete dbImg;
	}

	static void PlotMatches(
		Image<Byte> &aDbImage,
		Image<Byte> &aQueryImage, 
		FeatureStore &aDbFeatures, 
		FeatureStore &aQueryFeatures,
		vector< pair<Int,Int> > &aMatches, 
		Char *aOutFile)
	{
		vector<Int> inliers;
		Int numMatches = aMatches.size();
		for (Int i = 0; i < numMatches; ++i) inliers.push_back(i);
		PlotMatches(aDbImage, aQueryImage, aDbFeatures, aQueryFeatures, aMatches, aOutFile, inliers);
	}
	static void PlotMatches(
		Image<Byte> &aDbImage,
		Image<Byte> &aQueryImage, 
		FeatureStore &aDbFeatures, 
		FeatureStore &aQueryFeatures,
		vector< pair<Int,Int> > &aMatches, 
		Char *aOutFile,
		vector<Int> &aInliers) 
	{
		Image<Byte> *qImg = &aQueryImage;
		Image<Byte> *dbImg = &aDbImage;

		Int qW = qImg->Width();
		Int qH = qImg->Height();

		Int dbW = dbImg->Width();
		Int dbH = dbImg->Height();

		// create mosaic
		Int h = qH + dbH;
		Int w = qW;
		if (dbW > qW) w = dbW;

		Int chan = 3;
		Image<Byte> outImg(w, h, chan);

		for (Int i = 0; i < dbW; ++i) {
			for (Int j = 0; j < dbH; ++j) {
				Byte *in = dbImg->PixelPointer(i, j);
				Byte *out = outImg.PixelPointer(i, j);
				for (Int k = 0; k < chan; ++k) {
					out[k] = in[0];
				}
			}
		}
		for (Int i = 0; i < qW; ++i) {
			for (Int j = 0; j < qH; ++j) {
				Byte *in = qImg->PixelPointer(i, j);
				Byte *out = outImg.PixelPointer(i, j+dbH);
				for (Int k = 0; k < chan; ++k) {
					out[k] = in[0];
				}
			}
		}

		// make colors
		vector<Byte> red;
		red.push_back(255);
		red.push_back(0);
		red.push_back(0);

		vector<Byte> blue;
		blue.push_back(0);
		blue.push_back(0);
		blue.push_back(255);

		vector<Byte> green;
		green.push_back(0);
		green.push_back(255);
		green.push_back(0);

		// plot features
		for (Int i = 0; i < aDbFeatures.Size(); ++i) {
			Frame dbFrame = aDbFeatures.GetFrame(i);
			PlotFrame(outImg, dbFrame, green);
		}
		for (Int i = 0; i < aQueryFeatures.Size(); ++i) {
			Frame qFrame = aQueryFeatures.GetFrame(i);
			qFrame[1] += dbH;
			PlotFrame(outImg, qFrame, green);
		}

		// plot all matches
		Int numMatches = aMatches.size();
		for (Int i = 0; i < numMatches; ++i) {
			Int qIdx = aMatches[i].first;
			Int dbIdx = aMatches[i].second;

			Frame dbFrame = aDbFeatures.GetFrame(dbIdx);
			Frame qFrame = aQueryFeatures.GetFrame(qIdx);

			qFrame[1] += dbH;

			PlotMatch(outImg, qFrame, dbFrame, red);
			PlotFrame(outImg, qFrame, red);
			PlotFrame(outImg, dbFrame, red);
		}

		// plot inlier matches
		Int numInliers = aInliers.size();
		for (Int i = 0; i < numInliers; ++i) {
			Int qIdx = aMatches[aInliers[i]].first;
			Int dbIdx = aMatches[aInliers[i]].second;

			Frame dbFrame = aDbFeatures.GetFrame(dbIdx);
			Frame qFrame = aQueryFeatures.GetFrame(qIdx);

			qFrame[1] += dbH;

			PlotMatch(outImg, qFrame, dbFrame, blue);
			PlotFrame(outImg, qFrame, red);
			PlotFrame(outImg, dbFrame, red);
		}


		// output
		ImageIO::WritePPM(aOutFile, &outImg);
	}

	static void PlotMatch(Image<Byte> &aImage, Frame &aFrame1, Frame &aFrame2, vector<Byte> &aColor) {
		Int x1 = Int(aFrame1[0]);
		Int y1 = Int(aFrame1[1]);

		Int x2 = Int(aFrame2[0]);
		Int y2 = Int(aFrame2[1]);

		aImage.DrawLine(x1, y1, x2, y2, aColor);
	}

	static void PlotFrame(Image<Byte> &aImage, Frame &aFrame, vector<Byte> &aColor) {
		Float scaleMag = 12.0;

		Int x = Int(aFrame[0]);
		Int y = Int(aFrame[1]);
		Int scl = Int(scaleMag*aFrame[2]);
		Float ori = aFrame[3];

		aImage.DrawBox(x, y, scl, scl, ori, aColor);
	}
};

#endif
