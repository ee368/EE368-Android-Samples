#ifndef TRACKER_H
#define TRACKER_H

#include <string>

#include "cbir/RifTrack.h"
#include "cbir/BruteForce.h"
#include "cbir/L1Distance.h"
#include "cbir/MatchVisualizer.h"
#include "cbir/Ransac.h"
#include "cbir/Font.h"

class Tracker {
	// local typedef
	typedef vector< pair<Float, Float> > Polygon;

public:
	Tracker () {
		Construct();
	}

	void Construct() {
		iVisualize = false;
	}
	
	Bool TrackFrame(Image<Byte> &aImage) {
		Bool tracked = iTracker.TrackFrame(aImage);

		TrackPolygons();

		if (iVisualize) Visualize(aImage);

		return tracked;
	}

	void DrawPolygons(Image<Byte> &aImage) {
		vector<Byte> white(1);
		white[0] = 255;

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

				aImage.DrawLine(x0, y0, x1, y1, white);
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

			Int fw = aImage.iGlyphs.iFontWidth;
			Int fh = aImage.iGlyphs.iFontHeight;

			Int labelLen = iLabels[i].size();

			Int xOffset = fw * labelLen / 2;
			Int yOffset = fh / 2;

			Char *str = (Char *)iLabels[i].c_str();
			aImage.Text(xMean-xOffset, yMean-yOffset, str, white);
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

			Int fw = aImage.iGlyphs.iFontWidth;
			Int fh = aImage.iGlyphs.iFontHeight;

			Int labelLen = iLabels[i].size();

			Int xOffset = fw * labelLen / 2;
			Int yOffset = fh / 2;

			Char *str = (Char *)iLabels[i].c_str();
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

	void UpdatePolygons(vector< vector<Float> > &aModels, 
			vector< Image<Byte> > &aDbImages,
			vector<string> &aLabels) {

		Int numDB = aModels.size();

		for (Int i = 0; i < numDB; ++i) {
			// see if we are already tracking
			Bool isTracked = false;
			Int numPoly = iPolyIDs.size();
			for (Int j = 0; j < numPoly; ++j) {
				if (iPolyIDs[j] == i) isTracked = true;
			}

			Bool hasModel = aModels[i].size();

			if (hasModel && !isTracked) {
				// create database polygon
				Int w = aDbImages[i].Width();
				Int h = aDbImages[i].Height();

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
				iLabels.push_back(aLabels[i]);
			}
		}
	}

	void Reset() {
		iPolygons.clear();
		iPolyIDs.clear();
		iLabels.clear();
	}

public:
	// parameters
	Bool iVisualize;

	// function objects
	RifTrack iTracker;

	// label data
	vector<Polygon> iPolygons;
	vector<IDType> iPolyIDs;
	vector<string> iLabels;

	Glyphs iGlyphs;

public:
	const static Int KX = 0;
	const static Int KY = 1;
	const static Int KScl = 2;
	const static Int KOri = 3;
	const static Int KRes = 4;
};

#endif
