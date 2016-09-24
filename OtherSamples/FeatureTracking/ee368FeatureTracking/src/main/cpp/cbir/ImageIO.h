#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#ifdef DISABLE_STL_IO

// dummy version of class for android
class ImageIO {
public:
	static Int WritePPM(const Char *aFileName, Image<Byte> &aImage) { return 0; }
	static Int WritePPM(const Char *aFileName, Image<Byte> *aImage) { return 0; }
	static Int WriteHDR(const Char *aFileName, Image<Float> &aImage) { return 0; }
	static Int WriteHDR(const Char *aFileName, Image<Float> *aImage) { return 0; }
	static Int WriteBinaryHDR(const Char *aFileName, Image<Float> &aImage) { return 0;} 
	static Int WriteBinaryHDR(const Char *aFileName, Image<Float> *aImage) { return 0; }
	static Int WritePGM(const Char *aFileName, Image<Float> &aImage) { return 0; }
	static Int WritePGM(const Char *aFileName, Image<Float> *aImage) { return 0; }
	static Int WritePGM(const Char *aFileName, Image<Byte> &aImage) { return 0; }
	static Int WritePGM(const Char *aFileName, Image<Byte> *aImage) { return 0; }
	static Image<Byte> *ReadPPM(const Char *aFileName) { return NULL; }
	static Image<Byte> *ReadPGM(const Char *aFileName) { return NULL; }
};

#else

#include <fstream>
#include <iostream>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cbir/types.h"
#include "cbir/Image.h"

class ImageIO {
public:
	static Int WritePPM(const Char *aFileName, Image<Byte> &aImage) {
		return WritePPM(aFileName, &aImage);
	}
	static Int WritePPM(const Char *aFileName, Image<Byte> *aImage) {
		ofstream outFile(aFileName, ios::binary);
                if (!outFile.good()) {
                        perror("Could not open file to write to");
                        return -1;
                }

		Int width = aImage->Width();
		Int height =  aImage->Height();

                outFile << "P6\n";
                outFile << width << " " << height << endl;
                outFile << "255\n"; 

                for (Int j = 0; j < height; ++j) {
                        for (Int i = 0; i < width; ++i) {
				Byte *pixel = aImage->PixelPointer(i, j);
				Char *byte = reinterpret_cast<Char *>(pixel);
                                outFile.write(&byte[0], 1);
                                outFile.write(&byte[1], 1);
                                outFile.write(&byte[2], 1);
                        }
                }

                outFile.close();
		return 0;
	}

	static Int WriteHDR(const Char *aFileName, Image<Float> &aImage) {
		return WriteHDR(aFileName, &aImage);
	}
	static Int WriteHDR(const Char *aFileName, Image<Float> *aImage) {
		ofstream outFile(aFileName);
                if (!outFile.good()) {
                        perror("Could not open file to write to");
                        return -1;
                }

		Int width = aImage->Width();
		Int height =  aImage->Height();
		Int chan = aImage->NumChan();

                for (Int j = 0; j < height; ++j) {
                        for (Int i = 0; i < width; ++i) {
				Float *pixel = aImage->PixelPointer(i, j);
				for (Int k = 0; k < chan; ++k) {
                                	outFile << pixel[k] << " ";
				}
                        }
			outFile << endl;
                }

                outFile.close();
		return 0;
	}

	static Int WriteBinaryHDR(const Char *aFileName, Image<Float> &aImage) {
		return WriteBinaryHDR(aFileName, &aImage);
	}
	static Int WriteBinaryHDR(const Char *aFileName, Image<Float> *aImage) {
		ofstream outFile(aFileName, ios::binary);
                if (!outFile.good()) {
                        perror("Could not open file to write to");
                        return -1;
                }

		Int width = aImage->Width();
		Int height =  aImage->Height();
		Int chan = aImage->NumChan();

		Char *byte;
		byte = reinterpret_cast<Char *>(&width);
		outFile.write(byte, sizeof(Int));
		byte = reinterpret_cast<Char *>(&height);
		outFile.write(byte, sizeof(Int));
		byte = reinterpret_cast<Char *>(&chan);
		outFile.write(byte, sizeof(Int));

                for (Int j = 0; j < height; ++j) {
                        for (Int i = 0; i < width; ++i) {
				Float *pixel = aImage->PixelPointer(i, j);
				for (Int k = 0; k < chan; ++k) {
					Char *byte = reinterpret_cast<Char *>(&pixel[k]);
					outFile.write(byte, sizeof(Float));
				}
                        }
                }

                outFile.close();
		return 0;
	}

	static Int WritePGM(const Char *aFileName, Image<Float> &aImage) {
		return WritePGM(aFileName, &aImage);
	}
	static Int WritePGM(const Char *aFileName, Image<Float> *aImage) {
		Image<Byte> img;
		aImage->Convert(img);
		return WritePGM(aFileName, &img);
	}

	static Int WritePGM(const Char *aFileName, Image<Byte> &aImage) {
		return WritePGM(aFileName, &aImage);
	}
	static Int WritePGM(const Char *aFileName, Image<Byte> *aImage) {
		ofstream outFile(aFileName, ios::binary);
                if (!outFile.good()) {
                        perror("Could not open file to write to");
                        return -1;
                }

		Int width = aImage->Width();
		Int height =  aImage->Height();

                outFile << "P5\n";
                outFile << width << " " << height << endl;
                outFile << "255\n"; 

		if (aImage->NumChan() > 1) {
	                for (Int j = 0; j < height; ++j) {
	                        for (Int i = 0; i < width; ++i) {
					Byte *pixel = aImage->PixelPointer(i, j);
					Char *byte = reinterpret_cast<Char *>(pixel);
	                                outFile.write(&byte[0], 1);
	                        }
	                }
		} else {
			Byte *pixels = aImage->PixelPointer(0, 0);
			Char *data = reinterpret_cast<Char *>(pixels);
			outFile.write(data, width*height);
		}

                outFile.close();
                return 0;
	}

	static Image<Byte> *ReadPPM(const Char *aFileName) {
		ifstream inFile(aFileName, ios::binary);
                if (!inFile.good()) {
                        perror("Could not open input file");
                        return NULL;
                }

                Int i, j;
                Int w, h, max;
                Char buffer[512];
                Char value;

                // get identifier
                inFile >> buffer;

                // look for ppm idenifier
                if (strcmp("P6", buffer)) {
                        perror("This file does not seem to be an binary ppm");
                        return NULL;
                }

                // throw away comments
                while (true) {
                        inFile >> buffer;
                        if (buffer[0] == '#') {
                                inFile.getline(buffer, sizeof(buffer));
                        } else {
                                break;
                        }
                }
                // get width
                w = atoi(buffer);

                // throw away comments
                while (true) {
                        inFile >> buffer;
                        if (buffer[0] == '#') {
                                inFile.getline(buffer, sizeof(buffer));
                        } else {
                                break;
                        }
                }
                // get height
                h = atoi(buffer);

                // throw away comments
                while (true) {
                        inFile >> buffer;
                        if (buffer[0] == '#') {
                                inFile.getline(buffer, sizeof(buffer));
                        } else {
                                break;
                        }
                }
                // get max
                max = atoi(buffer);

                // throw away next character
                inFile.read(buffer, 1);
		
		// sanity check
                if (w < 0 || h < 0 || max > 255 || max < 0) {
                        perror("Image parameters are out of range");
                        return NULL;
                }
        
		// make image array
		Image<Byte> *image = new Image<Byte>(w, h, 3);

                // read image array from file
                for (j = 0; j < h; j++) {
                        for (i = 0; i < w; i++) {
				Byte *pixel = image->PixelPointer(i, j);

                                // get red
                                inFile.read(&value, 1);
                                pixel[0] = Int(255*Float(value)/max);

                                // get green
                                inFile.read(&value, 1);
                                pixel[1] = Int(255*Float(value)/max);

                                // get blue
                                inFile.read(&value, 1);
                                pixel[2] = Int(255*Float(value)/max);
                        }
                }

		return image;
	}

	static Image<Byte> *ReadPGM(const Char *aFileName) {
		ifstream inFile(aFileName, ios::binary);
                if (!inFile.good()) {
                        perror("Could not open input file");
                        return NULL;
                }

                Int i, j;
                Int w, h, max;
                Char buffer[512];
                Char value;

                // get identifier
                inFile >> buffer;

                // look for ppm idenifier
                if (strcmp("P5", buffer)) {
                        perror("This file does not seem to be an binary ppm");
                        return NULL;
                }

                // throw away comments
                while (true) {
                        inFile >> buffer;
                        if (buffer[0] == '#') {
                                inFile.getline(buffer, sizeof(buffer));
                        } else {
                                break;
                        }
                }
                // get width
                w = atoi(buffer);

                // throw away comments
                while (true) {
                        inFile >> buffer;
                        if (buffer[0] == '#') {
                                inFile.getline(buffer, sizeof(buffer));
                        } else {
                                break;
                        }
                }
                // get height
                h = atoi(buffer);

                // throw away comments
                while (true) {
                        inFile >> buffer;
                        if (buffer[0] == '#') {
                                inFile.getline(buffer, sizeof(buffer));
                        } else {
                                break;
                        }
                }
                // get max
                max = atoi(buffer);

                // throw away next character
                inFile.read(buffer, 1);
		
		// sanity check
                if (w < 0 || h < 0 || max > 255 || max < 0) {
                        perror("Image parameters are out of range");
                        return NULL;
                }
        
		// make image array
		Image<Byte> *image = new Image<Byte>(w, h, 1);

                // read image array from file
                for (j = 0; j < h; j++) {
                        for (i = 0; i < w; i++) {
				Byte *pixel = image->PixelPointer(i, j);

                                // get red
                                inFile.read(&value, 1);
                                pixel[0] = Int(255*Float(value)/max);
                        }
                }

		return image;
	}
private:
};

#endif	// DISABLE_STL_IO

#endif
