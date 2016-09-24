#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <vector>

using namespace std;
using namespace cv;

extern "C" {
JNIEXPORT void JNICALL Java_edu_stanford_ee368_edgeslinescircles_EdgesLinesCirclesActivity_NativeCanny(JNIEnv*, jobject, jlong addrGray, jlong addrRgba);
JNIEXPORT void JNICALL Java_edu_stanford_ee368_edgeslinescircles_EdgesLinesCirclesActivity_NativeHoughLines(JNIEnv*, jobject, jlong addrGray, jlong addrRgba);
JNIEXPORT void JNICALL Java_edu_stanford_ee368_edgeslinescircles_EdgesLinesCirclesActivity_NativeHoughCircles(JNIEnv*, jobject, jlong addrGray, jlong addrRgba);

JNIEXPORT void JNICALL Java_edu_stanford_ee368_edgeslinescircles_EdgesLinesCirclesActivity_NativeCanny(JNIEnv*, jobject, jlong addrGray, jlong addrRgba) {
    Mat& matGray  = *(Mat*)addrGray;
    Mat& matRgb = *(Mat*)addrRgba;

    // Perform Canny edge detection
    Mat matGrayCanny;
    Canny(matGray, matGrayCanny, 10, 100);

    // Copy edge image into output
    cvtColor(matGrayCanny, matRgb, CV_GRAY2RGB); // BGR
}

JNIEXPORT void JNICALL Java_edu_stanford_ee368_edgeslinescircles_EdgesLinesCirclesActivity_NativeHoughLines(JNIEnv*, jobject, jlong addrGray, jlong addrRgba) {
    Mat& matGray  = *(Mat*)addrGray;
    Mat& matRgb = *(Mat*)addrRgba;

    // Resize to half resolution for faster processing
    Mat matGraySmall;
    int resizefactor = 2;
    resize(matRgb, matGraySmall, Size(matGray.cols/resizefactor, matGray.rows/resizefactor));

    // Perform Canny edge detection
    Mat matGraySmallCanny;
    Canny(matGraySmall, matGraySmallCanny, 20, 200);

    // Detect lines by Hough transform and draw
    vector<Vec4i> lines;
    HoughLinesP(matGraySmallCanny, lines, 1, CV_PI/180, 80, 30, 10);
    Point offset(-resizefactor, -resizefactor);
    for (int n = 0; n < lines.size(); n++) {
        Point pt1(resizefactor*lines[n][0], resizefactor*lines[n][1]);
        Point pt2(resizefactor*lines[n][2], resizefactor*lines[n][3]);
        line(matRgb, pt1 + offset, pt2 + offset, Scalar(0,0,0), 3, 8);
        line(matRgb, pt1, pt2, Scalar(255,255,0), 3, 8);
    }
}

JNIEXPORT void JNICALL Java_edu_stanford_ee368_edgeslinescircles_EdgesLinesCirclesActivity_NativeHoughCircles(JNIEnv*, jobject, jlong addrGray, jlong addrRgba) {
    Mat& matGray  = *(Mat*)addrGray;
    Mat& matRgb = *(Mat*)addrRgba;

    // Resize to half resolution for faster processing
    Mat matGraySmall;
    int resizefactor = 2;
    resize(matGray, matGraySmall, Size(matGray.cols/resizefactor, matGray.rows/resizefactor));

    // Smooth the image so that not too many circles are detected
    GaussianBlur(matGraySmall, matGraySmall, Size(5,5), 2, 2);

    // Detect circles by Hough transform and draw
    vector<Vec3f> circles;
    HoughCircles(matGraySmall, circles, CV_HOUGH_GRADIENT, 2, matGraySmall.rows/8, 200, 100,
                 matGraySmall.cols/25, matGraySmall.cols/6);
    Point offset(-resizefactor, -resizefactor);
    for (int n = 0; n < circles.size(); n++) {
        Point center(cvRound(resizefactor*circles[n][0]), cvRound(resizefactor*circles[n][1]));
        int radius = cvRound(resizefactor*circles[n][2]);

        // Draw circle center
        circle(matRgb, center + offset, 3, Scalar(0,0,0), -1, 8, 0);
        circle(matRgb, center, 3, Scalar(255,255,0), -1, 8, 0);

        // Draw circle outline
        circle(matRgb, center + offset, radius, Scalar(0,0,0), 3, 8, 0);
        circle(matRgb, center, radius, Scalar(255,255,0), 3, 8, 0);
    }
}
}
