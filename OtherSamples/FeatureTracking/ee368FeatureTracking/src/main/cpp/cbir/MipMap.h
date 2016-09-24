#ifndef MIP_MAP_H
#define MIP_MAP_H

#include "cbir/types.h"
#include "cbir/Fixed.h"
#include <stdlib.h>
#include <iostream>

template <class T_type>
class MipMap {
public:
	~MipMap(){}
	MipMap(){}
	MipMap(Image<T_type> &aImage) {
		Construct(aImage);
	}

	void Construct(Image<T_type> &aImage) {
		ConstructFast(aImage);
		//ConstructAccurate(aImage);
	}

	void ConstructFast(Image<T_type> &aImage) {
		Int height = aImage.Height();	
		Int width = aImage.Width();
		Int chan = aImage.NumChan();

		// Allocate mipmap layers, rouding up
		Float maxDim = Max(width, height);
		Int numMipMaps = (Int) ceil( log(maxDim) * KInvLog2 );	// log_2(maxDim)
		iImages.resize(numMipMaps);
		
		// Compute total allocation space for each individual layer 
		Int curWidth = width;
		Int curHeight = height;
		for (Int level = 0; level < numMipMaps; ++level) {
			// alloc layer
			iImages[level].Construct(curWidth, curHeight, chan);

			// update width values
			curWidth >>= 1;	// divide by 2
			if (curWidth & 1) ++curWidth;	// add 1 if odd

			// update height values
			curHeight >>= 1;	// divide by 2
			if (curHeight & 1) ++curHeight;	// add 1 if odd
		}

		// initialize layer 0
		iImages[0].Copy(aImage);

		// Compute the actual mipmap
		// loop over levels of the MipMap
		for (Int level = 1; level < numMipMaps; level++) {

			Int curWidth = iImages[level].Width();
			Int curHeight = iImages[level].Height();

			Int prevWidth = iImages[level-1].Width();
			Int prevHeight = iImages[level-1].Height();
			
			Float wx0, wx1, wx2, wy0, wy1, wy2;	// weights for 2x-1, 2x, 2x+1, and 2y-1, 2y, 2y+1
			Int x0, x1, x2, y0, y1, y2;		// Indices for 2x-1, 2x, 2x+1, and 2y-1, 2y, 2y+1
			
			for (Int y = 0; y < curHeight; ++y) {	// loop over y-position

				// compute weights
				// Round up or power of two check here
				if (prevHeight & 1) {	// Odd: Round up
					wy0 = Float(y) / Float(prevHeight);
					wy1 = Float(curHeight) / Float(prevHeight);
					wy2 = 1.0 - wy0 - wy1;
				} else {		// Even: No rounding
					wy0 = 0.25;
					wy1 = 0.5;
					wy2 = 0.25;
				}

				y0 = Max(2*y-1, 0);
				y1 = Min(2*y, prevHeight-1);
				y2 = Min(2*y+1, prevHeight-1);
	
				for (Int x = 0; x < curWidth; ++x) {	// loop over x-position

					// compute weights
					if (prevWidth & 1) {	// Odd: Round up
						wx0 = Float(x) / Float(prevWidth);
						wx1 = Float(curWidth) / Float(prevWidth);
						wx2 = 1.0 - wx0 - wx1;
					} else {		// Even: No rounding
						wx0 = 0.25;
						wx1 = 0.5;
						wx2 = 0.25;
					}

					x0 = Max(2*x-1, 0);
					x1 = Min(2*x, prevWidth-1);
					x2 = Min(2*x+1, prevWidth-1);

					for (Int c = 0; c < chan; ++c) {	// loop over channels
						Float val = 0.0;

						val += wy0 * (
								wx0 * iImages[level-1](x0, y0, c) +
								wx1 * iImages[level-1](x1, y0, c) +
								wx2 * iImages[level-1](x2, y0, c)
							     );

						val += wy1 * (
								wx0 * iImages[level-1](x0, y1, c) +
								wx1 * iImages[level-1](x1, y1, c) +
								wx2 * iImages[level-1](x2, y1, c)
							     );

						val += wy2 * (
								wx0 * iImages[level-1](x0, y2, c) +
								wx1 * iImages[level-1](x1, y2, c) +
								wx2 * iImages[level-1](x2, y2, c)
							     );

						iImages[level](x, y, c) = (T_type) val;
					}	// end chan

				}	// end x
			}	// end y
			#if 0
			Char name[256];
			sprintf(name, "level%d.pgm", level);
			ImageIO::WritePGM(name, &iImages[level]);
			#endif
		}	// end level
	}

	void ConstructAccurate(Image<T_type> &aImage) {

		Float sigma = iGaussianFactor * 2.0;

		Int height = aImage.Height();	
		Int width = aImage.Width();
		Int chan = aImage.NumChan();

		// Allocate mipmap layers, rouding up
		Float maxDim = Max(width, height);
		Int numMipMaps = (Int) ceil( log(maxDim) * KInvLog2 );	// log_2(maxDim)
		iImages.resize(numMipMaps);
		
		// Compute total allocation space for each individual layer 
		Int curWidth = width;
		Int curHeight = height;
		for (Int level = 0; level < numMipMaps; level++) {
			// alloc layer
			iImages[level].Construct(curWidth, curHeight, chan);

			// update width values
			if (curWidth % 2 == 0) {
			       curWidth = curWidth/2;		// even
			} else {
			       curWidth = curWidth/2 + 1;	// odd
			}

			// update height values
			if (curHeight % 2 == 0) {
				curHeight = curHeight/2;	// even
			} else {
				curHeight= curHeight/2 + 1;	// odd
			}
		}

		// initialize layer 0
		iImages[0].Copy(aImage);

		// Compute the actual mipmap
		// loop over levels of the MipMap
		for (Int level = 1; level < numMipMaps; level++) {
			Image<Byte> tempImage;
			tempImage.Copy(iImages[level-1]);

			// gaussian blur the image 
			tempImage.GaussianBlur(sigma);

			Int curWidth = iImages[level].Width();
			Int curHeight = iImages[level].Height();

			Int prevWidth = iImages[level-1].Width();
			Int prevHeight = iImages[level-1].Height();
			
			for (Int y = 0; y < curHeight; ++y) {	// loop over y-position

				for (Int x = 0; x < curWidth; ++x) {	// loop over x-position

					for (Int c = 0; c < chan; ++c) {	// loop over channels
						Float val = tempImage(2*x, 2*y, c);						

						iImages[level](x, y, c) = (T_type) val;
					}	// end chan

				}	// end x
			}	// end y
			#if 0
			Char name[256];
			sprintf(name, "level%d.pgm", level);
			ImageIO::WritePGM(name, &iImages[level]);
			#endif
		}	// end level
	}

	void ConstructMipMap( Int width, Int height, Int chan) {
		// Allocate mipmap layers, rouding up
		Float maxDim = Max(width, height);
		Int numMipMaps = (Int) ceil( log(maxDim) * KInvLog2 );	// log_2(maxDim)
		iImages.resize(numMipMaps);
		
		// Compute total allocation space for each individual layer 
		Int curWidth = width;
		Int curHeight = height;
		for (Int level = 0; level < numMipMaps; ++level) {
			// alloc layer
			iImages[level].Construct(curWidth, curHeight, chan);

			// update width values
			curWidth >>= 1;	// divide by 2
			if (curWidth & 1) ++curWidth;	// add 1 if odd

			// update height values
			curHeight >>= 1;	// divide by 2
			if (curHeight & 1) ++curHeight;	// add 1 if odd
		}
	}

	//void ConstructAccurate(Image<T_type> &aImage) {
	void CopyToLayer( Image<T_type> &aImage, Int level){
		// initialize layer 0
		iImages[level].Copy(aImage);
	}

	void BuildMipMapLayersSingleChannel() {
		if ( iImages[0].NumChan() != 1 ) return;
		
		Int numMipMaps = iImages.size(); 

		// Compute the actual mipmap
		// loop over levels of the MipMap
		for (Int level = 1; level < numMipMaps; level++) {

			Int curWidth = iImages[level].Width();
			Int curHeight = iImages[level].Height();

			Int prevWidth = iImages[level-1].Width();
			Int prevHeight = iImages[level-1].Height();
			
//			Float wx0, wx1, wx2, wy0, wy1, wy2;	// weights for 2x-1, 2x, 2x+1, and 2y-1, 2y, 2y+1
			TFixed wx0, wx1, wx2, wy0, wy1, wy2;	// weights for 2x-1, 2x, 2x+1, and 2y-1, 2y, 2y+1
			Int x0, x1, x2, y0, y1, y2;		// Indices for 2x-1, 2x, 2x+1, and 2y-1, 2y, 2y+1

			T_type* pDest = iImages[level].ScanLine(0);
			
			for (Int y = 0; y < curHeight; ++y) {	// loop over y-position

				// compute weights
				// Round up or power of two check here
				if (prevHeight & 1) {	// Odd: Round up
					wy0 = Float(y) / Float(prevHeight);
					wy1 = Float(curHeight) / Float(prevHeight);
					wy2 = 1.0 - wy0 - wy1;
				} else {		// Even: No rounding
					wy0 = 0.25;
					wy1 = 0.5;
					wy2 = 0.25;
				}

				y0 = Max(2*y-1, 0);
				y1 = Min(2*y, prevHeight-1);
				y2 = Min(2*y+1, prevHeight-1);

				T_type*	pRow0 = iImages[level-1].ScanLine( y0);
				T_type*	pRow1 = iImages[level-1].ScanLine( y1);
				T_type*	pRow2 = iImages[level-1].ScanLine( y2);
	
				for (Int x = 0; x < curWidth; ++x) {	// loop over x-position

					// compute weights
					if (prevWidth & 1) {	// Odd: Round up
						wx0 = Float(x) / Float(prevWidth);
						wx1 = Float(curWidth) / Float(prevWidth);
						wx2 = 1.0 - wx0 - wx1;
					} else {		// Even: No rounding
						wx0 = 0.25;
						wx1 = 0.5;
						wx2 = 0.25;
					}

					x0 = Max(2*x-1, 0);
					x1 = Min(2*x, prevWidth-1);
					x2 = Min(2*x+1, prevWidth-1);

					TFixed val = 0.0;

					val += wy0 * (
							wx0 * pRow0[x0] +
							wx1 * pRow0[x1] +
							wx2 * pRow0[x2]
						     );

					val += wy1 * (
							wx0 * pRow1[x0] +
							wx1 * pRow1[x1] +
							wx2 * pRow1[x2]
						     );

					val += wy2 * (
							wx0 * pRow2[x0] +
							wx1 * pRow2[x1] +
							wx2 * pRow2[x2]
						     );

//					iImages[level](x, y, 0) = (T_type) val;
					Int	nVal = val.Round();
					*(pDest++) = (T_type) nVal;
				}	// end x
			}	// end y
		}	// end level
	}


	T_type GetTrilinear(Float aX, Float aY, Float aScale, Int aChan) {
		Bool inBounds;
		return GetTrilinear(aX, aY, aScale, aChan, inBounds);
	}
	T_type GetTrilinear(Float aX, Float aY, Float aScale, Int aChan, Bool &aInBounds) {
	
		if (aScale < 1)	aScale = 1;		// Do not allow scale smaller than 1.
	
		// only compute log if it has changed
		static Float prevScale = 0;
		static Float logScale = 0;
		static Int scaleIdx[2];
		static Int scale[2];

		static Float heightRatio0 = 0;
		static Float widthRatio0 = 0;
		static Float heightRatio1 = 0;
		static Float widthRatio1 = 0;

		static Float weights[2];

		// only compute if scale has changed
		if (prevScale != aScale) {
			logScale = log((Float) aScale)*KInvLog2;	// log_2(scale)
			prevScale = aScale;

			// compute layer indices
			scaleIdx[0] = (Int) floor(logScale);
			scaleIdx[1] = (Int) ceil(logScale);

			// compute scales for layers
			scale[0] = 1 << scaleIdx[0];
			scale[1] = 1 << scaleIdx[1];

			// base images size
			Int height = iImages[0].Height();
			Int width = iImages[0].Width();

			// size of larger image
			Int height0 = iImages[scaleIdx[0]].Height();
			Int width0 = iImages[scaleIdx[0]].Width();

			// size of smaller image
			Int height1 = iImages[scaleIdx[1]].Height();
			Int width1 = iImages[scaleIdx[1]].Width();

			// scaling ratios
			heightRatio0 = height0 / Float(height);
			heightRatio1 = height1 / Float(height);
			widthRatio0 = width0 / Float(width);
			widthRatio1 = width1 / Float(width);

			// compute weights
			Float totalScale = Float(scale[1] - scale[0]);
			weights[0] = Float(scale[1] - aScale) / totalScale;
			weights[1] = 1.0 - weights[0];
		}

		// adjust location to local position
		Float x0 = aX * widthRatio0;
		Float y0 = aY * heightRatio0;
		Float x1 = aX * widthRatio1;
		Float y1 = aY * heightRatio1;
	
		// bounds checking
		aInBounds = true;
		if (x0 < 0 || x0 > iImages[scaleIdx[0]].Width()) aInBounds = false;
		if (y0 < 0 || y0 > iImages[scaleIdx[0]].Height()) aInBounds = false;
		if (x1 < 0 || x1 > iImages[scaleIdx[1]].Width()) aInBounds = false;
		if (y1 < 0 || y1 > iImages[scaleIdx[1]].Height()) aInBounds = false;

		// compute value
		Float value = 0;
		if (scaleIdx[0] == scaleIdx[1]) {
			value = iImages[scaleIdx[0]].GetBilinearPixel(x0, y0, aChan);
		} else {
			value += weights[0] * iImages[scaleIdx[0]].GetBilinearPixel(x0, y0, aChan);
			value += weights[1] * iImages[scaleIdx[1]].GetBilinearPixel(x1, y1, aChan);
		}

		return (T_type) value;
	}
	T_type GetTrilinearFixed(TFixed aX, TFixed aY, TFixed aScale, Int aChan) {
	
		if (aScale < 1)	
			aScale = 1;		// Do not allow scale smaller than 1.
	
		// only compute log if it has changed
		static TFixed prevScale = 0;
		static Float logScale = 0;
		static Int scaleIdx[2];
		static Int scale[2];

		static TFixed heightRatio0 = 0;
		static TFixed widthRatio0 = 0;
		static TFixed heightRatio1 = 0;
		static TFixed widthRatio1 = 0;

		static TFixed weights[2];

		// only compute if scale has changed
		if (prevScale != aScale) {
			logScale = log((Float) aScale.Real())*KInvLog2;	// log_2(scale)
			prevScale = aScale;

			// compute layer indices
			scaleIdx[0] = (Int) floor(logScale);
			scaleIdx[1] = (Int) ceil(logScale);

			// compute scales for layers
			scale[0] = 1 << scaleIdx[0];
			scale[1] = 1 << scaleIdx[1];

			// base images size
			Int height = iImages[0].Height();
			Int width = iImages[0].Width();

			// size of larger image
			Int height0 = iImages[scaleIdx[0]].Height();
			Int width0 = iImages[scaleIdx[0]].Width();

			// size of smaller image
			Int height1 = iImages[scaleIdx[1]].Height();
			Int width1 = iImages[scaleIdx[1]].Width();

			// scaling ratios
			heightRatio0 = height0 / Float(height);
			heightRatio1 = height1 / Float(height);
			widthRatio0 = width0 / Float(width);
			widthRatio1 = width1 / Float(width);

			// compute weights
			Float totalScale = Float(scale[1] - scale[0]);
			weights[0] = Float(scale[1] - aScale.Real()) / totalScale;
			weights[1] = 1 - weights[0];
		}

		// adjust location to local position
		TFixed x0 = aX * widthRatio0;
		TFixed y0 = aY * heightRatio0;
		TFixed x1 = aX * widthRatio1;
		TFixed y1 = aY * heightRatio1;
	
		// compute value
		TFixed value = 0;
		if (scaleIdx[0] == scaleIdx[1]) {
			value = iImages[scaleIdx[0]].GetBilinearPixelFixed(x0, y0, aChan);
		} else {
			value += weights[0] * iImages[scaleIdx[0]].GetBilinearPixelFixed(x0, y0, aChan);
			value += weights[1] * iImages[scaleIdx[1]].GetBilinearPixelFixed(x1, y1, aChan);
		}

		return (T_type) value.Real();
	}
	T_type GetTrilinearFixed(TFixed aX, TFixed aY, TFixed aScale) {
	
		if (aScale < 1)	
			aScale = 1;		// Do not allow scale smaller than 1.
	
		// only compute log if it has changed
		static TFixed prevScale = 0;
		static Float logScale = 0;
		static Int scaleIdx[2];
		static Int scale[2];

		static TFixed heightRatio0 = 0;
		static TFixed widthRatio0 = 0;
		static TFixed heightRatio1 = 0;
		static TFixed widthRatio1 = 0;

		static TFixed weights[2];

		// only compute if scale has changed
		if (prevScale != aScale) {
			logScale = log((Float) aScale.Real())*KInvLog2;	// log_2(scale)
			prevScale = aScale;

			// compute layer indices
			scaleIdx[0] = (Int) floor(logScale);
			scaleIdx[1] = (Int) ceil(logScale);

			// compute scales for layers
			scale[0] = 1 << scaleIdx[0];
			scale[1] = 1 << scaleIdx[1];

			// base images size
			Int height = iImages[0].Height();
			Int width = iImages[0].Width();

			// size of larger image
			Int height0 = iImages[scaleIdx[0]].Height();
			Int width0 = iImages[scaleIdx[0]].Width();

			// size of smaller image
			Int height1 = iImages[scaleIdx[1]].Height();
			Int width1 = iImages[scaleIdx[1]].Width();

			// scaling ratios
			heightRatio0 = height0 / Float(height);
			heightRatio1 = height1 / Float(height);
			widthRatio0 = width0 / Float(width);
			widthRatio1 = width1 / Float(width);

			// compute weights
			Float totalScale = Float(scale[1] - scale[0]);
			weights[0] = Float(scale[1] - aScale.Real()) / totalScale;
			weights[1] = 1 - weights[0];
		}

		// adjust location to local position
		TFixed x0 = aX * widthRatio0;
		TFixed y0 = aY * heightRatio0;
		TFixed x1 = aX * widthRatio1;
		TFixed y1 = aY * heightRatio1;
	
		// compute value
		TFixed value = 0;
		if (scaleIdx[0] == scaleIdx[1]) {
			value = iImages[scaleIdx[0]].GetBilinearPixelFixed(x0, y0);
		} else {
			value += weights[0] * iImages[scaleIdx[0]].GetBilinearPixelFixed(x0, y0);
			value += weights[1] * iImages[scaleIdx[1]].GetBilinearPixelFixed(x1, y1);
		}

		return (T_type) value.Real();
	}
	

	template <class S>
	void Interpolate(
		Float aScale,
		vector<Float> &aX, 
		vector<Float> &aY, 
		vector<Int> &aU, 
		vector<Int> &aV, 
		Image<S> &aZ,
		Int aWidth = -1,
		Int aHeight = -1) 
	{
		Int uMin = 0;
		Int vMin = 0;
		Int uMax = 0;
		Int vMax = 0;
		if (aWidth < 0 || aHeight < 0) {
			vector<Int>::iterator iter; 
			iter = max_element(aU.begin(), aU.end());
			uMax = *iter;
			iter = min_element(aU.begin(), aU.end());
			uMin = *iter;
			iter = max_element(aV.begin(), aV.end());
			vMax = *iter;
			iter = min_element(aV.begin(), aV.end());
			vMin = *iter;
	
			aWidth = uMax-uMin+1;
			aHeight = vMax-vMin+1;
		} 

		Int numChan = iImages[0].NumChan();
		aZ.Construct(aWidth, aHeight, numChan);

		Int size = aX.size();
		for (Int i = 0; i < size; ++i) {
			for (Int j = 0; j < numChan; ++j) {
				aZ(aU[i]+uMin, aV[i]+vMin, j) = (S) GetTrilinear(aX[i], aY[i], aScale, j);
			}
		}
	}
	template <class S>
	void InterpolateFixed(
		TFixed aScale,
		vector<TFixed > &aX, 
		vector<TFixed > &aY, 
		vector<Int> &aU, 
		vector<Int> &aV, 
		Image<S> &aZ,
		Int aWidth = -1,
		Int aHeight = -1) 
	{
		Int uMin = 0;
		Int vMin = 0;
		Int uMax = 0;
		Int vMax = 0;
		if (aWidth < 0 || aHeight < 0) {
			vector<Int>::iterator iter; 
			iter = max_element(aU.begin(), aU.end());
			uMax = *iter;
			iter = min_element(aU.begin(), aU.end());
			uMin = *iter;
			iter = max_element(aV.begin(), aV.end());
			vMax = *iter;
			iter = min_element(aV.begin(), aV.end());
			vMin = *iter;
	
			aWidth = uMax-uMin+1;
			aHeight = vMax-vMin+1;
		} 

		Int numChan = iImages[0].NumChan();
		aZ.Construct(aWidth, aHeight, numChan);

		Int size = aX.size();
		for (Int i = 0; i < size; ++i) {
			for (Int j = 0; j < numChan; ++j) {
				aZ(aU[i]+uMin, aV[i]+vMin, j) = (S) GetTrilinearFixed(aX[i], aY[i], aScale, j);
			}
		}
	}

private:

	template <class S>
	inline S Min(S a, S b) {
		if (a < b) return a;
		return b;
	}
	template <class S>
	inline S Max(S a, S b) {
		if (a > b) return a;
		return b;
	}
	
public:
	vector<Image<T_type> > iImages;

	// This is 1/log_e(2)
	static const Float KInvLog2 = 1.442695040888963f;
	const static Float iGaussianFactor = 0.65;	// gaussian blur sigma per downsample amount

};

#endif
