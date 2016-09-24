#ifndef LDRIMAGE_H
#define LDRIMAGE_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "cbir/stl.h"
#include "cbir/types.h"
#include "cbir/print.h"
#include "cbir/Fixed.h"
#include "cbir/Font.h"
#include "cbir/RingBuffer.h"

template <class T>
class Image {
public:
	Image() {
		Init();
	}
	
	Image(Int aWidth, Int aHeight, Int aChan) {
		Init();
		Construct(aWidth, aHeight, aChan);
	}
	
	virtual ~Image() {
		Destroy();
	}

	void Init() {
		iChan = 0;
		iWidth = 0;
		iHeight = 0;
		iArray = NULL;
	}

	void Destroy() {
		if (iArray) delete[] iArray;
		Init();
	}
	
	void Construct(Int aWidth, Int aHeight, Int aChan, Bool aZero = true) {
		if (iWidth == aWidth && 
		    iHeight == aHeight && 
		    iChan == aChan) {
			return;
		}

		Destroy();

		iChan = aChan;
		iWidth = aWidth;
		iHeight = aHeight;
		
		Int size = iChan * iHeight * iWidth;

		if (size > 0) {
			iArray = new T[size];
			if (aZero) Zero();
		} else {
			Init();
		}
	}

	template <class S>
	void Copy(Image<S> &aImage) {
		Copy(&aImage);
	}

	template <class S>
	void Copy(Image<S> *aImage) {
		Construct(aImage->Width(), 
			aImage->Height(), 
			aImage->NumChan());

		Int size = iChan * iHeight * iWidth;
		S *aArray = aImage->ScanLine(0);
		for (Int i = size-1; i >= 0; --i) {
			iArray[i] = (T) aArray[i];
		}
	}

	Int NumChan() { return iChan; }
	Int Width() { return iWidth; }
	Int Height() { return iHeight; }

	T GetBoxFilter(Int aX1, Int aY1, Int aX2, Int aY2) {
		return (*this)(aX2, aY2) - (*this)(aX1, aY2) - (*this)(aX2, aY1) + (*this)(aX1, aY1);
	}

	T *PixelPointer(Int aX, Int aY) {
		return PixelPointer(aX, aY, 0);
	}
	T *PixelPointer(Int aX, Int aY, Int aC) {
		if (aY >= iHeight || aY < 0) return NULL;
		if (aX >= iWidth || aX < 0) return NULL;
		Int offset = (aY*iWidth + aX) * iChan;
		T *p = &iArray[offset];
		return &p[aC];
	}
	T *ScanLine(Int aY) {
		if (aY >= iHeight || aY < 0) return NULL;
		Int offset = aY * iWidth * iChan;
		return &iArray[offset];
	}

	void MinMax(T *aMin, T *aMax) {
		T max = ns::numeric_limits<T>::min();
		T min = ns::numeric_limits<T>::max();
	
		Int size = iChan * iHeight * iWidth;
		for (Int i = size-1; i >= 0; --i) {
			if (iArray[i] > max) max = iArray[i];
			if (iArray[i] < min) min = iArray[i];
		}

		*aMin = min;
		*aMax = max;
	}

	void Zero() { Fill(0); }
	void Fill(T aValue) {
		Int size = iChan * iHeight * iWidth;
		for (Int i = size-1; i >= 0; --i) {
			iArray[i] = aValue;
		}
	}

	// bit reverse the first n bits of an integer
	static Uint BitReverse(Uint x, Uint n) {
		Int numBits = sizeof(Uint) << 3;

		Uint y = 0;
		for (Int i = 0; i < 8*sizeof(x) && i < n; i++) {
			y <<= 1;
			y += (x >> i) & 1;
		}
		
		return y;
	}

	template <class S>
	static void HammingWindow(vector<S> &x) {
		static const Double pi = 3.14159265358979;

		Int N = x.size();
		if (N <= 0) return;

		for (Int n = 0; n < N; ++n) {
			x[n] *= 0.54 - 0.46 * cos( 2*pi * n / (N-1) );
		}
	}
	
	// arrange vector in bit reversed order
	template <class S>
	static void BitReverseVect(vector<S> &x) {
		Int n = x.size();
		vector<S> tmp(n);
	
		Int bits = (Int)ceil( log(n)/log(2) );
		
		// reorder
		for (Int i = 0; i < n; ++i) {
			tmp[BitReverse(i, bits)] = x[i];
		}
	
		// copy data back to x
		x = tmp;
	}

	// input must be power of 2 length, otherwise data is truncated to next smallest power of 2
	// inverse true for ifft
	template <class S>
	static void FFT(vector<S> &real, vector<S> &imag, Bool inverse = false) {
		static const Double pi = 3.14159265358979;

		Int N = real.size();
		if (N <= 0) return;

		// only use power of two poInts	
		Int bits = 0;
		for (bits = 0; (1 << bits) <= N; ++bits );
		--bits;
		Int n = 1 << bits;			// crop vectors
	
		// arrange the input vector in bit reversed order
		BitReverseVect(real);
		BitReverseVect(imag);

		// perform butterfly process
		for (Int i = 0; i < bits; ++i) {		// for each log N stage
			Int sectSize = (Int)pow(2.0, Double(i));	// section size of butterfly
		
			// traverse each n component
			for (Int k = 0; k < n-1; k += 2*sectSize) {
			
				// traverse sections
				for (Int j = 0; j < sectSize; ++j) {
					S arg = -pi*j/sectSize;
				
					if (inverse) arg = -arg;
				
					S c = cos(arg);
					S s = sin(arg);
					
					Int m = k + j;

					S tmpReal = real[m+sectSize]*c-imag[m+sectSize]*s;
					S tmpImag = imag[m+sectSize]*c+real[m+sectSize]*s;

					real[m+sectSize] = real[m] - tmpReal;
					imag[m+sectSize] = imag[m] - tmpImag;

					real[m] += tmpReal;
					imag[m] += tmpImag;
				}
			}
		}

		S nSqrtInv = 1.0 / sqrt(n);
		for (Int i = 0; i < n; ++i) {
			real[i] *= nSqrtInv;
			imag[i] *= nSqrtInv;
		}
	}

	// 2D fourier transform
	static void FFT(Image<Float> &aReal, Image<Float> &aImaginary, Bool aInverse = false) {
		Int w = aReal.Width();
		Int h = aReal.Height();
		Int chan = aReal.NumChan();

		if (aImaginary.Width() != w || aImaginary.Height() != h || aImaginary.NumChan() != chan) {
			printf("Size mismatch\n");
			return;
		}

		// do each channel independently
		for (Int c = 0; c < chan; ++c) {

			// transform each column
			for (Int x = 0; x < w; ++x) {

				vector<Float> real(h);
				vector<Float> imaginary(h);

				for (Int y = 0; y < h; ++y) {
					real[y] = aReal(x, y, c);
					imaginary[y] = aImaginary(x, y, c);
				}

				FFT(real, imaginary, aInverse);

				for (Int y = 0; y < h; ++y) {
					aReal(x, y, c) = real[y];
					aImaginary(x, y, c) = imaginary[y];
				}
			}

			// transform each row
			for (Int y = 0; y < h; ++y) {

				vector<Float> real(w);
				vector<Float> imaginary(w);

				for (Int x = 0; x < w; ++x) {
					real[x] = aReal(x, y, c);
					imaginary[x] = aImaginary(x, y, c);
				}

				FFT(real, imaginary, aInverse);

				for (Int x = 0; x < w; ++x) {
					aReal(x, y, c) = real[x];
					aImaginary(x, y, c) = imaginary[x];
				}
			}

		}
	}

	Image *Resize(Int aW, Int aH) {
		if (aW < 1 || aH < 1) {
			return NULL;
		}

		Int oldw = iWidth;
		Int oldh = iHeight;

		Float scalex = Float(aW) / Float(oldw);
		Float scaley = Float(aH) / Float(oldh);

		// create new image
		Image *newImg = new Image(aW, aH, iChan);

		for (Int j = 0; j < aH; j++) {
			for (Int i = 0; i < aW; i++) {

				// position in old image;
				Float x = i / scalex;
				Float y = j / scaley;
		
				// reference pixel in old image
				Int p = (Int) x;
				Int q = (Int) y;

				// coordinates relative to neighboring pixels
				Float u = x - p;
				Float v = y - q;
				
				T *a = PixelPointer(p  , q  );
				T *b = PixelPointer(p+1, q  );
				T *c = PixelPointer(p  , q+1);
				T *d = PixelPointer(p+1, q+1);

				if (!a || !b || !c || !d) continue;

				T *pixel = newImg->PixelPointer(i, j);

				for (Int k = 0; k < iChan; k++) {
					pixel[k] = (T) BilinearInterp(a[k], b[k], c[k], d[k], u, v);
				}
			}
		}

		return newImg;
	}

	void ComputeIntegralImage() {

		// Horizontal pass
		for (Int y = 0; y < iHeight; ++y) {
			for (Int x = 1; x < iWidth; ++x) {
				for (Int c = 0; c < iChan; ++c) {
					(*this)(x, y, c) += (*this)(x-1, y, c);					
				}
			}
		}

		// Vertical pass
		for (Int y = 1; y < iHeight; ++y) {
			for (Int x = 0; x < iWidth; ++x) {
				for (Int c = 0; c < iChan; ++c) {
					(*this)(x, y, c) += (*this)(x, y-1, c);
				}
			}
		}

	}

	void ComputeDerivativeImage() {

		// Horizontal pass
		for (Int y = 0; y < iHeight; ++y) {
			for (Int x = iWidth-1; x > 0; --x) {
				for (Int c = 0; c < iChan; ++c) {
					(*this)(x, y, c) -= (*this)(x-1, y, c);
				}
			}
		}

		// Vertical pass
		for (Int y = iHeight-1; y > 0; --y) {
			for (Int x = 0; x < iWidth; ++x) {
				for (Int c = 0; c < iChan; ++c) {
					(*this)(x, y, c) -= (*this)(x, y-1, c);
				}
			}
		}

	}

#if 0
	void HorizShift(Int aOffset) {
		if (aOffset == 0) return;

		if (aOffset > 0) {
			for (Int j = iHeight-1; j >= 0; --j) {
				// copy scan line
				for (Int i = iWidth-1-aOffset; i >= 0; --i) {			
					T *src = PixelPointer(i, j);
					T *dst = PixelPointer(i+aOffset, j);

					for (Int k = iChan-1; k >= 0; --k) {
						dst[k] = src[k];
					}
				}

				// zero remaining pixels
				for (Int i = aOffset-1; i >= 0; --i) {			
					T *dst = PixelPointer(i, j);
					for (Int k = iChan-1; k >= 0; --k) {
						dst[k] = 0;
					}
				}
			}
		} else {
			for (Int j = iHeight-1; j >= 0; --j) {
				// copy scan line
				for (Int i = -aOffset; i < iWidth; ++i) {			
					T *src = PixelPointer(i, j);
					T *dst = PixelPointer(i+aOffset, j);

					for (Int k = iChan-1; k >= 0; --k) {
						dst[k] = src[k];
					}
				}

				// zero remaining pixels
				for (Int i = iWidth+aOffset; i < iWidth; ++i) {			
					T *dst = PixelPointer(i, j);
					for (Int k = iChan-1; k >= 0; --k) {
						dst[k] = 0;
					}
				}

			}
		}
	}
#else
	void HorizShift(Int aOffset) {
		if (aOffset == 0) return;

		if (aOffset > 0) {
			for (Int j = 0; j < iHeight; ++j) {

				// copy scan line
				for (Int k = 0; k < iChan; ++k) {
					for (Int i = iWidth-1-aOffset; i >= 0; --i) {			
						(*this)(i+aOffset, j, k) = (*this)(i, j, k);
					}
				}

				// zero remaining pixels
				for (Int k = 0; k < iChan; ++k) {
					for (Int i = aOffset-1; i >= 0; --i) {			
						(*this)(i, j, k) = 0;
					}
				}
			}
		} else {
			for (Int j = 0; j < iHeight; ++j) {

				// copy scan line
				for (Int k = 0; k < iChan; ++k) {
					for (Int i = -aOffset; i < iWidth; ++i) {			
						(*this)(i+aOffset, j, k) = (*this)(i, j, k);
					}
				}

				// zero remaining pixels
				for (Int k = 0; k < iChan; ++k) {
					for (Int i = iWidth+aOffset; i < iWidth; ++i) {			
						(*this)(i, j, k) = 0;
					}
				}

			}
		}
	}
#endif

	void VertShift(Int aOffset) {
		if (aOffset == 0) return;

		if (aOffset > 0) {
			for (Int i = iWidth-1; i >= 0; --i) {			
				for (Int j = iHeight-1-aOffset; j >= 0; --j) {
				// copy scan line
					T *src = PixelPointer(i, j);
					T *dst = PixelPointer(i, j+aOffset);

					for (Int k = iChan-1; k >= 0; --k) {
						dst[k] = src[k];
					}
				}

				// zero remaining pixels
				for (Int j = aOffset-1; j >= 0; --j) {			
					T *dst = PixelPointer(i, j);
					for (Int k = iChan-1; k >= 0; --k) {
						dst[k] = 0;
					}
				}
			}
		} else {
			for (Int i = iWidth-1; i >= 0; --i) {			
			 	for (Int j = -aOffset; j < iHeight; ++j) {
				// copy scan line

					T *src = PixelPointer(i, j);
					T *dst = PixelPointer(i, j+aOffset);

					for (Int k = iChan-1; k >= 0; --k) {
						dst[k] = src[k];
					}
				}

				// zero remaining pixels
				for (Int j = iHeight+aOffset; j < iHeight; ++j) {			
					T *dst = PixelPointer(i, j);
					for (Int k = iChan-1; k >= 0; --k) {
						dst[k] = 0;
					}
				}

			}
		}
	}

	void Shift(Int aX, Int aY) {
		HorizShift(aX);
		VertShift(aY);
	}

	void CircularShift(Int aX, Int aY) {
		// make sure parameters are non-negative
		while (aX < 0) aX += iWidth;
		while (aY < 0) aY += iHeight;

		Image<T> temp;
		temp.Copy(*this);

		for (Int j = 0; j < iHeight; ++j) {
			for (Int i = 0; i < iWidth; ++i) {
				for (Int k = 0; k < iChan; ++k) {
					Int ii = (i + aX) % iWidth;
					Int jj = (j + aY) % iHeight;
					(*this)(i, j, k) = temp(ii, jj, k);
				}
			}
		}
	}

	void HorizBlur(Int aSize) {
		if (aSize <= 1) return;

		Int len = (aSize-1) >> 1;
		Int size = 2*len + 1;
		Float sizeInv = 1.0 / size;

		// create black pixel
		T *black = new T[iChan];
		for (Int i = 0; i < iChan; i++) {
			black[i] = 0;
		}

		// sum of values per channel in the buffer
		Int *accum = new Int[iChan];

		// buffer of pixels under filter so that we can filter in place
		Int bufIdx = 0;
		Int bufSize = iChan * size + 1;
		T *buf = new T[bufSize];

		for (Int j = 0; j < iHeight; ++j) {
			// initialize
			bufIdx = 0;
			for (Int k = 0; k < iChan; ++k) accum[k] = 0;
			for (Int i = 0; i < bufSize; ++i) buf[i] = 0;

			// preload the buffer
			T *row = &(*this)(0, j);
			for (Int i = 0; i < len; ++i) {
				for (Int k = 0; k < iChan; ++k) {
					buf[bufIdx] = row[bufIdx];
					accum[k] += buf[bufIdx];
					++bufIdx;
				}
			}

			// filter the row
			for (Int i = 0; i < iWidth; ++i) {
				Int outIdx = bufIdx + iChan;
				if (outIdx >= bufSize) outIdx = 0;
				T *out = &buf[outIdx];

				T *in;
				if (i + len < iWidth) {
					in = &(*this)(i+len, j);
				} else {
					in = black;
				}

				T *pixel = &(*this)(i, j);

				for (Int k = iChan-1; k >= 0; --k) {
					buf[bufIdx] = in[k];
					accum[k] = accum[k] + in[k] - out[k];
					pixel[k] = (T) (accum[k] * sizeInv);	// optimize this
					++bufIdx;
				}

				if (bufIdx >= bufSize) bufIdx = 0;
			}
		}

		delete[] black;
		delete[] accum;
		delete[] buf;
	}

	void HorizBlurPow2(Int aSize) {
		if (aSize <= 1) return;

		// compute log 2 of size
		Int log2Size;
		Int tempSize = aSize;
		for (log2Size = 0; tempSize > 1; ++log2Size) {
			tempSize >>= 1;
		}

		Int size = (1 << log2Size);
		Int len = size >> 1;

		// create black pixel
		T *black = new T[iChan];
		for (Int i = 0; i < iChan; i++) {
			black[i] = 0;
		}

		// sum of values per channel in the buffer
		Int *accum = new Int[iChan];

		// buffer of pixels under filter so that we can filter in place
		Int bufIdx = 0;
		Int bufSize = iChan * size + 1;
		T *buf = new T[bufSize];

		for (Int j = 0; j < iHeight; ++j) {
			// initialize
			bufIdx = 0;
			for (Int k = 0; k < iChan; ++k) accum[k] = 0;
			for (Int i = 0; i < bufSize; ++i) buf[i] = 0;

			// preload the buffer
			T *row = &(*this)(0, j);
			for (Int i = 0; i < len; ++i) {
				for (Int k = 0; k < iChan; ++k) {
					buf[bufIdx] = row[bufIdx];
					accum[k] += buf[bufIdx];
					++bufIdx;
				}
			}

			// filter the row
			for (Int i = 0; i < iWidth; ++i) {
				Int outIdx = bufIdx + iChan;
				if (outIdx >= bufSize) outIdx = 0;
				T *out = &buf[outIdx];

				T *in;
				if (i + len < iWidth) {
					in = &(*this)(i+len, j);
				} else {
					in = black;
				}

				T *pixel = &(*this)(i, j);

				//for (Int k = iChan-1; k >= 0; --k) {
				for (Int k = 0; k < iChan; ++k) {
					buf[bufIdx] = in[k];
					accum[k] = accum[k] + in[k] - out[k];
					pixel[k] = (T) (accum[k] >> log2Size);
					++bufIdx;
				}

				if (bufIdx-bufSize >= 0) bufIdx = 0;
			}
		}

		delete[] black;
		delete[] accum;
		delete[] buf;
	}

	void VertBlur(Int aSize) {
		if (aSize <= 1) return;

		Int len = (aSize-1) >> 1;
		Int size = 2*len + 1;
		Float sizeInv = 1.0 / size;

		T *black = new T[iChan];
		for (Int i = 0; i < iChan; i++) {
			black[i] = 0;
		}

		// sum of values per channel in the buffer
		Int *accum = new Int[iChan];

		// buffer of pixels under filter so that we can filter in place
		Int bufIdx = 0;
		Int bufSize = iChan * size + 1;
		T *buf = new T[bufSize];

		for (Int j = 0; j < iWidth; ++j) {
			// initialize
			bufIdx = 0;
			for (Int k = 0; k < iChan; ++k) accum[k] = 0;
			for (Int i = 0; i < bufSize; ++i) buf[i] = 0;

			// preload the buffer
			for (Int i = 0; i < len; ++i) {
				T *pixel = PixelPointer(j, i);
				for (Int k = 0; k < iChan; ++k) {
					buf[bufIdx] = pixel[k];
					accum[k] += buf[bufIdx];
					++bufIdx;
				}
			}

			for (Int i = 0; i < iHeight; ++i) {
				Int outIdx = bufIdx + iChan;
				if (outIdx >= bufSize) outIdx = 0;
				T *out = &buf[outIdx];

				T *in;
				if (i + len < iHeight) {
					in = PixelPointer(j, i+len);
				} else {
					in = black;
				}

				T *pixel = PixelPointer(j, i);

				for (Int k = 0 ; k < iChan; ++k) {
					buf[bufIdx] = in[k];
					accum[k] = accum[k] + in[k] - out[k];
					pixel[k] = (T) (accum[k] * sizeInv);
					++bufIdx;
				}

				if (bufIdx >= bufSize) bufIdx = 0;	
			}
		}

		delete[] black;
		delete[] accum;
		delete[] buf;
	}

	void VertBlurPow2(Int aSize) {
		if (aSize <= 1) return;

		// compute log 2 of size
		Int log2Size;
		Int tempSize = aSize;
		for (log2Size = 0; tempSize > 1; ++log2Size) {
			tempSize >>= 1;
		}

		Int size = (1 << log2Size);
		Int len = size >> 1;

		// create black pixel
		T *black = new T[iChan];
		for (Int i = 0; i < iChan; i++) {
			black[i] = 0;
		}

		// sum of values per channel in the buffer
		Int *accum = new Int[iChan];

		// buffer of pixels under filter so that we can filter in place
		Int bufIdx = 0;
		Int bufSize = iChan * size + 1;
		T *buf = new T[bufSize];

		for (Int j = 0; j < iWidth; ++j) {
			// initialize
			bufIdx = 0;
			for (Int k = 0; k < iChan; ++k) accum[k] = 0;
			for (Int i = 0; i < bufSize; ++i) buf[i] = 0;

			// preload the buffer
			for (Int i = 0; i < len; ++i) {
				T *pixel = &(*this)(j, i);
				for (Int k = 0; k < iChan; ++k) {
					buf[bufIdx] = pixel[k];
					accum[k] += buf[bufIdx];
					++bufIdx;
				}
			}

			// filter column
			for (Int i = 0; i < iHeight; ++i) {
				Int outIdx = bufIdx + iChan;
				if (outIdx >= bufSize) outIdx = 0;
				T *out = &buf[outIdx];

				T *in;
				if (i + len < iHeight) {
					in = &(*this)(j, i+len);
				} else {
					in = black;
				}

				T *pixel = &(*this)(j, i);

				//for (Int k = iChan-1; k >= 0; --k) {
				for (Int k = 0; k < iChan; ++k) {
					buf[bufIdx] = in[k];
					accum[k] = accum[k] + in[k] - out[k];
					pixel[k] = (T) (accum[k] >> log2Size);
					++bufIdx;
				}

				if (bufIdx-bufSize >= 0) bufIdx = 0;
			}
		}

		delete[] black;
		delete[] accum;
		delete[] buf;
	}

	void Blur(Int aSize) { 
		HorizBlur(aSize); 
		VertBlur(aSize); 
	}

	void BlurPow2(Int aSize) { 
		HorizBlurPow2(aSize); 
		VertBlurPow2(aSize); 
	}


	void ExtractRegion(Int aXMin, Int aXMax, Int aYMin, Int aYMax, Image &aRegion) {	
		// reallocate region to proper size
		Int regionWidth = aXMax-aXMin+1;
		Int regionHeight = aYMax-aYMin+1;
		aRegion.Construct(regionWidth, regionHeight, iChan);

		// check boundary cases
		for (Int j = aYMin; j <= aYMax; ++j) {
			for (Int i = aXMin; i <= aXMax; ++i) {
				if (i < 0 || i >= iWidth || j < 0 || j >= iHeight ) continue;
				for (Int k = 0; k < iChan; ++k) {
					T *p = aRegion.PixelPointer(i-aXMin, j-aYMin, k);
					*p = (*this)(i,j,k);
				}
			}
		}
	}

	T GetBilinearPixel(Float aX, Float aY, Int aChan) {
		Int p = (Int) aX;
		Int q = (Int) aY;

		// coordinates relative to neighboring pixels
		Float u = aX - p;
		Float v = aY - q;
				
		// check bounds
		if (p+1 >= iWidth) return (T) 0;
		if (p < 0) return (T) 0;
		if (q+1 >= iHeight) return (T) 0;
		if (q < 0) return (T) 0;

		T a = (*this)(p  , q  , aChan);
		T b = (*this)(p+1, q  , aChan);
		T c = (*this)(p  , q+1, aChan);
		T d = (*this)(p+1, q+1, aChan);

		return (T) BilinearInterp(a, b, c, d, u, v);
	}
	TFixed GetBilinearPixelFixed(TFixed aX, TFixed aY, Int aChan) {
		Int p = aX.Truncate();
		Int q = aY.Truncate();

		// coordinates relative to neighboring pixels
		TFixed u = aX - p;
		TFixed v = aY - q;
				
		// check bounds
		if (p+1 >= iWidth) return (T) 0;
		if (p < 0) return (T) 0;
		if (q+1 >= iHeight) return (T) 0;
		if (q < 0) return (T) 0;

		T a = (*this)(p  , q  , aChan);
		T b = (*this)(p+1, q  , aChan);
		T c = (*this)(p  , q+1, aChan);
		T d = (*this)(p+1, q+1, aChan);

		TFixed af = a;
		TFixed bf = b;
		TFixed cf = c;
		TFixed df = d;

		return BilinearInterpFixed(af, bf, cf, df, u, v);
	}

	template <class S>
	void Interpolate(
		vector<Float> &aX, 
		vector<Float> &aY, 
		vector<Int> &aU, 
		vector<Int> &aV, 
		Image<S> &aZ) 
	{
		vector<Int>::iterator iter; 
		iter = max_element(aU.begin(), aU.end());
		Int uMax = *iter;
		iter = min_element(aU.begin(), aU.end());
		Int uMin = *iter;
		iter = max_element(aV.begin(), aV.end());
		Int vMax = *iter;
		iter = min_element(aV.begin(), aV.end());
		Int vMin = *iter;

		Int zWidth = uMax-uMin+1;
		Int zHeight = vMax-vMin+1;
		aZ.Construct(zWidth, zHeight, iChan);

		Int size = aX.size();
		for (Int i = 0; i < size; ++i) {
			Int p = (Int) aX[i];
			Int q = (Int) aY[i];

			// coordinates relative to neighboring pixels
			Float u = aX[i] - p;
			Float v = aY[i] - q;
				
			T *a = PixelPointer(p  , q  );
			T *b = PixelPointer(p+1, q  );
			T *c = PixelPointer(p  , q+1);
			T *d = PixelPointer(p+1, q+1);

			// check bounds
			if (!a || !b || !c || !d) continue;

			for (Int j = 0; j < iChan; ++j) {
				S *p = aZ.PixelPointer(aU[i], aV[i], j);
				*p = (S) BilinearInterp(*(a+j), *(b+j), *(c+j), *(d+j), u, v);
			}
		}
	}
	template <class S>
	void InterpolateFixed(
		vector< TFixed > &aX, 
		vector< TFixed > &aY, 
		vector<Int> &aU, 
		vector<Int> &aV, 
		Image<S> &aZ) 
	{
		vector<Int>::iterator iter; 
		iter = max_element(aU.begin(), aU.end());
		Int uMax = *iter;
		iter = min_element(aU.begin(), aU.end());
		Int uMin = *iter;
		iter = max_element(aV.begin(), aV.end());
		Int vMax = *iter;
		iter = min_element(aV.begin(), aV.end());
		Int vMin = *iter;

		Int zWidth = uMax-uMin+1;
		Int zHeight = vMax-vMin+1;
		aZ.Construct(zWidth, zHeight, iChan);

		Int size = aX.size();
		for (Int i = 0; i < size; ++i) {
			Int p = aX[i].Truncate();
			Int q = aY[i].Truncate();

			// coordinates relative to neighboring pixels
			Float u = aX[i] - p;
			Float v = aY[i] - q;
				
			T *a = PixelPointer(p  , q  );
			T *b = PixelPointer(p+1, q  );
			T *c = PixelPointer(p  , q+1);
			T *d = PixelPointer(p+1, q+1);

			// check bounds
			if (!a || !b || !c || !d) continue;

			for (Int j = 0; j < iChan; ++j) {
				S *p = aZ.PixelPointer(aU[i], aV[i], j);
		
				TFixed af = *(a+j);
				TFixed bf = *(b+j);
				TFixed cf = *(c+j);
				TFixed df = *(d+j);

				*p = (S) BilinearInterpFixed(af, bf, cf, df, u, v);
			}
		}
	}


	void ComputeSymGradients(Image &aGradX, Image &aGradY) {
		ComputeSymHorizGradient(aGradX);
		ComputeSymVertGradient(aGradY);
	}

	void ComputeSymHorizGradient(Image<Float> &aGrad) {
		aGrad.Construct(iWidth, iHeight, iChan);

		for (Int j = 0; j < iHeight; ++j) {
			for (Int i = 0; i < iWidth; ++i) {
				for (Int k = 0; k < iChan; ++k) {

					Float forward = (*this)(i, j, k);
					Float backward = forward;
					Float baselineInv = 1.0;

					if (i < iWidth-1) forward = (*this)(i+1, j, k);
					if (i > 0) backward = (*this)(i-1, j, k);
					if (i > 0 && i < iWidth-1) baselineInv = 0.5;

					aGrad(i, j, k) = (forward - backward) * baselineInv;
				}
			}
		}
	}

	void ComputeSymVertGradient(Image<Float> &aGrad) {
		aGrad.Construct(iWidth, iHeight, iChan);
	
		for (Int i = 0; i < iWidth; ++i) {
			for (Int j = 0; j < iHeight; ++j) {
				for (Int k = 0; k < iChan; ++k) {

					Float forward = (*this)(i, j, k);
					Float backward = forward;
					Float baselineInv = 1.0;

					if (j < iHeight-1) forward = (*this)(i, j+1, k);
					if (j > 0) backward = (*this)(i, j-1, k);
					if (j > 0 && j < iHeight-1) baselineInv = 0.5;

					aGrad(i, j, k) = (forward - backward) * baselineInv;
				}
			}
		}
	}

	void ComputeGradients(Image &aGradX, Image &aGradY) {
		ComputeHorizGradient(aGradX);
		ComputeVertGradient(aGradY);
	}

	void ComputeHorizGradient(Image<Float> &aGrad) {
		aGrad.Construct(iWidth, iHeight, iChan);

		for (Int j = 0; j < iHeight; ++j) {
			for (Int i = 0; i < iWidth; ++i) {
				for (Int k = 0; k < iChan; ++k) {
					T *outPixel = aGrad.PixelPointer(i, j, k);

					T cur = (*this)(i, j, k);

					T prev = 0;
					if (i > 0) prev = (*this)(i-1, j, k);

					*outPixel = cur - prev;
				}
			}
		}
	}

	void ComputeHorizGradient(Image<Byte> &aGrad) {
		aGrad.Construct(iWidth, iHeight, iChan);

		for (Int j = 0; j < iHeight; ++j) {
			for (Int i = 0; i < iWidth; ++i) {
				for (Int k = 0; k < iChan; ++k) {
					T *outPixel = aGrad.PixelPointer(i, j, k);

					T cur = (*this)(i, j, k);

					T prev = 0;
					if (i > 0) prev = (*this)(i-1, j, k);

					*outPixel = ((cur - prev) >> 1) + 128;
				}
			}
		}
	}

	void ComputeVertGradient(Image<Float> &aGrad) {
		aGrad.Construct(iWidth, iHeight, iChan);
	
		for (Int i = 0; i < iWidth; ++i) {
			for (Int j = 0; j < iHeight; ++j) {
				for (Int k = 0; k < iChan; ++k) {
					T *outPixel = aGrad.PixelPointer(i, j, k);

					T cur = (*this)(i, j, k);
	
					T prev = 0;
					if (j > 0) prev = (*this)(i, j-1, k);

					*outPixel = cur - prev;
				}
			}
		}
	}

	void ComputeVertGradient(Image<Byte> &aGrad) {
		aGrad.Construct(iWidth, iHeight, iChan);
	
		for (Int i = 0; i < iWidth; ++i) {
			for (Int j = 0; j < iHeight; ++j) {
				for (Int k = 0; k < iChan; ++k) {
					T *outPixel = aGrad.PixelPointer(i, j, k);

					T cur = (*this)(i, j, k);
	
					T prev = 0;
					if (j > 0) prev = (*this)(i, j-1, k);

					*outPixel = ((cur - prev) >> 1) + 128;
				}
			}
		}
	}

	void Normalize() {
		Float mean;
		Float var;

		Float newMean = 0;
		Float newStdDev = 1;

		for (Int k = 0; k < iChan; ++k) {
			Variance(k, mean, var);
			Float stdDev = sqrt(var);

			if (stdDev == 0) stdDev = 1;	// avoid divide by 0

			for (Int i = 0; i < iWidth; ++i) {
				for (Int j = 0; j < iHeight; ++j) {
					T *p = PixelPointer(i, j, k);
					Float val = (*p - mean) / stdDev;
					val = newStdDev * val + newMean;
					*p = (T) val;
				}
			}
		}
	}

	Float Mean(Int aChan) {
		Float mean = 0;
		for (Int j = 0; j < iHeight; ++j) {
			Float rowSum = 0;
			for (Int i = 0; i < iWidth; ++i) {
				rowSum += (*this)(i, j, aChan);
			}
			mean += rowSum / iWidth;
		}
		mean /= iHeight;
	
		return mean;
	}

	Float Variance(Int aChan) {
		Float var;
		Float mean;
		Variance(aChan, mean, var);
		return var;
	}

	void Variance(Int aChan, Float &aMean, Float &aVariance) {

		Float mean = Mean(aChan);

		Float variance = 0;
		for (Int j = 0; j < iHeight; ++j) {
			Float rowSum = 0;
			for (Int i = 0; i < iWidth; ++i) {
				Float dev = (*this)(i, j, aChan) - mean;
				rowSum += dev*dev;
			}
			variance += rowSum / iWidth;
		}
		variance /= iHeight;

		aMean = mean;
		aVariance = variance;
	}

	void DrawPoint(Float aX, Float aY, vector<T> aColor, Float aWidth) {
		Int colorDim = aColor.size();
		if (colorDim <= 0) return;

		for (Int i = (Int)-aWidth; i <= aWidth; ++i) {
			for (Int j = (Int)-aWidth; j <= aWidth; ++j) {

				Int x = (Int) aX + i;
				Int y = (Int) aY + j;

				if (x < 0 || y < 0 || x >= iWidth || y >= iHeight) continue;

				T *p = PixelPointer(x, y);
				for (Int k = 0; k < iChan; ++k) {
					if (k >= colorDim) {
						p[k] = aColor[colorDim-1];
					} else {
						p[k] = aColor[k];
					}
				}
			}
		}
	}

	void DrawLine(Int aX0, Int aY0, Int aX1, Int aY1, vector<T> aColor) {

		if (aX0 != aX1) {				
			Float slope = Float(aY1 - aY0) / Float(aX1 - aX0);

			if (slope <= 1 && slope >= -1) {	// abs(slope) <= 1
				if (aX1 < aX0) {
					Int tmp;
					tmp = aX0; aX0 = aX1; aX1 = tmp;					
					tmp = aY0; aY0 = aY1; aY1 = tmp;
				}
				
				for (Int x = aX0; x <= aX1; ++x) {
					Int y = (Int)( slope*(x-aX0) + aY0 );
					SetPixel(x, y, aColor);
				}
			} else {
				if (aY1 < aY0) {
					Int tmp;
					tmp = aX0; aX0 = aX1; aX1 = tmp;					
					tmp = aY0; aY0 = aY1; aY1 = tmp;
				}

				slope = 1.0 / slope;
				
				for (Int y = aY0; y <= aY1; ++y) {
					Int x = (Int)( slope*(y-aY0) + aX0 );
					SetPixel(x, y, aColor);
				}
			}
		} else {
			if (aY1 < aY0) {
				Int tmp;
				tmp = aX0; aX0 = aX1; aX1 = tmp;					
				tmp = aY0; aY0 = aY1; aY1 = tmp;
			}

			for (Int y = aY0; y <= aY1; ++y) {
				Int x = aX0;
				SetPixel(x, y, aColor);
			}
		}
	}

	void DrawBox(Int aX, Int aY, Int aW, Int aH, Float aTheta, vector<T> aColor) {
		Float bbx0 = - aW / 2.0;
		Float bby0 = - aH / 2.0;
		Float bbx1 = + aW / 2.0;
		Float bby1 = + aH / 2.0;

		Float ct = cos(aTheta);
		Float st = sin(aTheta);

		Int x0 = Int( ct * bbx0 - st * bby0 ) + aX;
		Int y0 = Int( st * bbx0 + ct * bby0 ) + aY;

		Int x1 = Int( ct * bbx0 - st * bby1 ) + aX;
		Int y1 = Int( st * bbx0 + ct * bby1 ) + aY;

		Int x2 = Int( ct * bbx1 - st * bby1 ) + aX;
		Int y2 = Int( st * bbx1 + ct * bby1 ) + aY;

		Int x3 = Int( ct * bbx1 - st * bby0 ) + aX;
		Int y3 = Int( st * bbx1 + ct * bby0 ) + aY;

		DrawLine(x0, y0, x1, y1, aColor);
		DrawLine(x1, y1, x2, y2, aColor);
		DrawLine(x2, y2, x3, y3, aColor);
		DrawLine(x3, y3, x0, y0, aColor);
	}

	void FastGaussianBlur(Float aSigma) {
		if (aSigma <= 0) return;

		Int blurSize = (Int) round(2 * aSigma);	// the 2 is a constant and should not be changed
		Blur(blurSize);
		Blur(blurSize);
		Blur(blurSize);
	}

	void GaussianBlur(Float aSigma) {
		if (aSigma <= 0) return;

		// create kernel
		Int support = 2*Int(round(2 * aSigma)) + 1;
		Int center = (support-1) / 2;

		// gaussian is separable, so do horizontal and vertical pass
		Image<Float> vertKernel(1, support, 1);
		Image<Float> horizKernel(support, 1, 1);

		Float sum = 0;
		for (Int x = 0; x < support; ++x) {
			Float dx = (x-center) / aSigma;
			Float value = exp(-dx*dx);

			vertKernel(0, x) = value;
			horizKernel(x, 0) = value;
			sum += value;
		}

		// normalize
		vertKernel.Multiply(1.0 / sum);
		horizKernel.Multiply(1.0 / sum);

		// perform convolution
		Convolve(horizKernel);
		Convolve(vertKernel);
	}

	template <class S>
	void Convolve(Image<S> &aKernel) {
		Image<T> original;
		original.Copy(this);

		Convolve(aKernel, original);
	}

	template <class S>
	void Convolve(Image<S> &aKernel, Image<T> &original) {
		Int w = aKernel.Width();
		Int h = aKernel.Height();

		if (w <= 0 || h <= 0) return;

		// approximate center of kernel
		Int cx = Int(w / 2.0);
		Int cy = Int(h / 2.0);

		// loop over pixels
		for (Int y = 0; y < iHeight; ++y) {
			for (Int x = 0; x < iWidth; ++x) {
				for (Int c = 0; c < iChan; ++c) {
					Float sum = 0;
					Float denom = 0;

					// loop over kernel
					for (Int ky = 0; ky < h; ++ky) {
						for (Int kx = 0; kx < w; ++kx) {
							Int xx = x + kx - cx;
							Int yy = y + ky - cy;

							if (xx < 0 || xx >= iWidth) continue;
							if (yy < 0 || yy >= iHeight) continue;

							sum += aKernel(kx, ky) * original(xx, yy, c);
							denom += aKernel(kx, ky);
						}
					}

					(*this)(x, y, c) = T(sum / denom);
				}
			}
		}
	}

	template <class S>
	void HorizConvolve(Image<S> &aKernel) {
		Int kw = aKernel.Width();
		Int w = iWidth;
		Int h = iHeight;
		Byte zero = 0;

		// approximate center
		Int center = kw >> 1;
		Int lookahead = kw-center+1;

		// create ring buffer for in-place convolve
		// one for each channel
		vector< RingBuffer<T> > buf(iChan);

		for (Int j = 0; j < h; ++j) {
			// clear the buffer
			for (Int c = 0; c < iChan; ++c) {
				buf[c].Construct(kw);
			}

			// preload buffer
			for (Int i = center; i < kw; ++i) {
				for (Int c = 0; c < iChan; ++c) {
					buf[c].Push( (*this)(i-center, j, c) );
				}
			}

			// filter row
			for (Int i = 0; i < w; ++i) {
				for (Int c = 0; c < iChan; ++c) {
					Int bs = buf[c].Size();

					// inner product
					Float sum = 0;
					Float denom = 0;
					for (Int k = 0; k < bs; ++k) {
						Int kIdx = k + kw - bs;
						Float kern = aKernel(kIdx, 0, c);
						sum += kern * buf[c][k];
						denom += kern;
					}
					(*this)(i, j, c) = sum / denom;

					// add to buffer
					if (i+lookahead < w) {
						buf[c].Push( (*this)(i+lookahead, j, c) );
					} else {
						buf[c].Push( zero );
					}
				}
			}
		}
	}

	template <class S>
	void VertConvolve(Image<S> &aKernel) {
		Int kw = aKernel.Height();
		Int w = iWidth;
		Int h = iHeight;
		Byte zero = 0;

		// approximate center
		Int center = kw >> 1;
		Int lookahead = kw-center+1;

		// create ring buffer for in-place convolve
		// one for each channel
		vector< RingBuffer<T> > buf(iChan);

		for (Int j = 0; j < w; ++j) {
			// clear the buffer
			for (Int c = 0; c < iChan; ++c) {
				buf[c].Construct(kw);
			}

			// preload buffer
			for (Int i = center; i < kw; ++i) {
				for (Int c = 0; c < iChan; ++c) {
					buf[c].Push( (*this)(j, i-center, c) );
				}
			}

			// filter row
			Int end = 0;
			for (Int i = 0; i < h; ++i) {
				for (Int c = 0; c < iChan; ++c) {
					Int bs = buf[c].Size();

					// inner product
					Float sum = 0;
					Float denom = 0;
					for (Int k = 0; k < bs-end; ++k) {
						Int kIdx = k + kw - bs;
						Float kern = aKernel(0, kIdx, c);
						sum += kern * buf[c][k];
						denom += kern;
					}
					(*this)(j, i, c) = sum / denom;


					// add to buffer
					if (i+lookahead < h) {
						buf[c].Push( (*this)(j, i+lookahead, c) );
						end = 0;
					} else {
						buf[c].Push( zero );
						end = i + lookahead - h;
					}
				}
			}
		}
	}

	void Crop(Int aX0, Int aY0, Int aX1, Int aY1) {
		if (aX0 > aX1) return;
		if (aY0 > aY1) return;
		
		// make working copy
		Image<T> oldImage;
		oldImage.Copy(*this);

		Int oldHeight = oldImage.Height();
		Int oldWidth = oldImage.Width();

		Int newWidth = aX1 - aX0;
		Int newHeight = aY1 - aY0;

		Construct(newWidth, newHeight, iChan);

		// loop over new image
		for (Int x = 0; x < iWidth; ++x) {
			for (Int y = 0; y < iHeight; ++y) {
				for (Int c = 0; c < iChan; ++c) {
					Int xx = x + aX0;
					Int yy = y + aY0;
			
					if (xx < 0) continue;
					if (yy < 0) continue;
					if (xx >= oldWidth) continue;
					if (yy >= oldHeight) continue;

					(*this)(x, y, c) = oldImage(xx, yy, c);
				}
			}
		}


	}

	void Multiply(Float aValue) {
		Int size = iChan * iHeight * iWidth;
		for (Int i = size-1; i >= 0; --i) {
			iArray[i] *= aValue;
		}
	}

	void Round() {
		Int size = iChan * iHeight * iWidth;
		for (Int i = size-1; i >= 0; --i) {
			iArray[i] = (Int) iArray[i];
		}
	}

	void SetPixel(Int aX, Int aY, vector<T> aColor) {
		Int colorDim = aColor.size();
		if (colorDim <= 0) return;

		if (aX < 0 || aY < 0 || aX >= iWidth || aY >= iHeight) return;

		T *p = PixelPointer(aX, aY);

		for (Int k = 0; k < iChan; ++k) {
			if (k >= colorDim) {
				p[k] = aColor[colorDim-1];
			} else {
				p[k] = aColor[k];
			}
		}
	}

	template <class S>
	void Convert(Image<S> &aOutput) {
		aOutput.Construct(iWidth, iHeight, iChan);

		S maxVal;
		TypeMax(maxVal);

		// find max and min
		T max, min;
		MinMax(&min, &max);
		Float delta = max - min;

		for (Int i = 0; i < iWidth; ++i) {
			for (Int j = 0; j < iHeight; ++j) {
				for (Int k = 0; k < iChan; ++k) {
					S *outPixel = aOutput.PixelPointer(i, j, k);
					T inValue = (*this)(i, j, k);

					Float outValue = maxVal * (inValue - min) / delta;
					*outPixel = (S) outValue;
				}
			}
		}
	}

	void Text(Int aX, Int aY, Char *aString, vector<T> aColor) {
		Int fw = iGlyphs.iFontWidth;
		Int fh = iGlyphs.iFontHeight;

		for (Int i = 0; aString[i]; ++i) {	// characters

			// loop over pixels in the character
			for (Int x = 0; x < fw; ++x) {
				for (Int y = 0; y < fh; ++y) {
					Int idx = x + y * fw;
					Bool bit = iGlyphs((Int)aString[i], idx);

					if (bit) {
						Int xPos = x + aX + i*fw;
						Int yPos = y + aY;
						if (xPos < 0 || xPos > iWidth-1) continue;
						if (yPos < 0 || yPos > iHeight-1) continue;

						for (Int c = 0; c < iChan; ++c) {
							(*this)(xPos, yPos, c) = aColor[c];
						}
					}
				}
			}
		}
	}
	
	// operators
	inline T &operator() (Int aX, Int aY) { 
		Int offset = aY*iWidth + aX;
		return iArray[offset];
	}
//	inline T &operator() (Int aX, Int aY) { return (*this)(aX,aY,0); }
	inline T &operator() (Int aX, Int aY, Int aC) { 
		Int offset = (aY*iWidth + aX) * iChan + aC;
		return iArray[offset];
	}
	
private:
	template <class S>
	void TypeMax(S &val) { val = ns::numeric_limits<S>::max(); }
	void TypeMax(Float &val) { val = 1.0; }
	void TypeMax(Double &val) { val = 1.0; }

	inline TFixed BilinearInterpFixed(TFixed a, TFixed b, TFixed c, TFixed d, TFixed u, TFixed v) {
		TFixed e = a + u*(b-a);
		TFixed f = c + u*(d-c);
	
		TFixed val = e + v*(f-e);
		return val;
	}
	inline Float BilinearInterp(Float a, Float b, Float c, Float d, Float u, Float v) {
		Float e = a + u*(b-a);
		Float f = c + u*(d-c);
	
		Float val = e + v*(f-e);
		return val;
	}

private:
	Int iChan;
	Int iWidth;
	Int iHeight;
	T *iArray;

public:
	Glyphs iGlyphs;
};

#endif

