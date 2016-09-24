#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "cbir/RifTrack.h"

using namespace std;
using namespace cv;

RifTrack* m_rifTrack = NULL;
Image<Byte>* m_frame = NULL;
unsigned int m_frameWidth = 0;
unsigned int m_frameHeight = 0;
unsigned int m_frameCount = 0;
unsigned int m_downsampleFactor = 2;
bool m_trackingValid = false;
char m_textBuffer[1024];
vector< vector<float> > m_matchedPoints;

void initializeTracker(unsigned int width, unsigned int height) {
    m_rifTrack = new RifTrack();
    m_frame = new Image<Byte>(width, height, 1);
    m_frameWidth = width;
    m_frameHeight = height;
}

unsigned int trackFrame(Mat matGray) {
    // Convert from CV image format to RIFF image format
    uchar* matGrayData = matGray.data;
    size_t matGrayStep = matGray.step;
    Byte* frameptr = m_frame->PixelPointer(0,0);
    for (int row = 0; row < matGray.rows; row++) {
        for (int col = 0; col < matGray.cols; col++) {
            frameptr[col] = matGrayData[col];
        }
        matGrayData += matGrayStep;
        frameptr += matGray.cols;
    } // row

    // Track frame
    m_trackingValid = m_rifTrack->TrackFrame(*m_frame, m_matchedPoints);

    // Return number of feature matches
    return m_rifTrack->iMatches.size();
}

void visualizeTracking(Mat& matRgb) {
    // Print number of tracked features
    unsigned int numFeatureMatches = m_rifTrack->iMatches.size();
    sprintf(m_textBuffer, "%02d features tracked", numFeatureMatches);
    int fontFace = FONT_HERSHEY_COMPLEX;
    double fontScale = 1;
    Point textOrigin(10,25);
    rectangle(matRgb, Point(0,0), Point(400,45), Scalar(0,0,0), CV_FILLED);
    putText(matRgb, string(m_textBuffer), textOrigin, fontFace, fontScale, Scalar(255,255,0));

    // Draw motion vectors
    Point offset(-2,-2);
    int radius = 5;
    float motionVectorExpansion = 2;
    for (int n = 0; n < m_matchedPoints.size(); n++) {
        vector<float>& matchedPoint = m_matchedPoints[n];
        Point prevPt(matchedPoint[0]*m_downsampleFactor, matchedPoint[1]*m_downsampleFactor);
        Point currPt(matchedPoint[2]*m_downsampleFactor, matchedPoint[3]*m_downsampleFactor);
        Point deltaPt = motionVectorExpansion*(prevPt - currPt);

        circle(matRgb, currPt + offset, radius, Scalar(0,0,0), 3);
        circle(matRgb, currPt - offset, radius, Scalar(0,0,0), 3);
        circle(matRgb, currPt, radius, Scalar(255,255,0), 3);

        line(matRgb, currPt + offset, currPt + deltaPt + offset, Scalar(0,0,0), 3);
        line(matRgb, currPt - offset, currPt + deltaPt - offset, Scalar(0,0,0), 3);
        line(matRgb, currPt, currPt + deltaPt, Scalar(255,255,0), 3);
    } // n
}

extern "C" {
JNIEXPORT void JNICALL Java_edu_stanford_ee368_featuretracking_FeatureTrackingActivity_NativeProcessing(JNIEnv*, jobject, jlong addrGray, jlong addrRgba);

JNIEXPORT void JNICALL Java_edu_stanford_ee368_featuretracking_FeatureTrackingActivity_NativeProcessing(JNIEnv*, jobject, jlong addrGray, jlong addrRgba) {
    Mat& matGray  = *(Mat*)addrGray;
    Mat& matRgb = *(Mat*)addrRgba;

    // Downsample image
    Mat matGraySmall;
    resize(matGray, matGraySmall, Size(matGray.cols/m_downsampleFactor, matGray.rows/m_downsampleFactor));

    // Initialize tracker
    if (m_frameCount == 0) {
        initializeTracker(matGraySmall.cols, matGraySmall.rows);
    }

    // Track frame
    trackFrame(matGraySmall);

    // Draw tracking visualization
    visualizeTracking(matRgb);
    m_frameCount++;
}
}
