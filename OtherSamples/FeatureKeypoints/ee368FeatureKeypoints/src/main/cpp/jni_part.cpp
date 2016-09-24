#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

using namespace std;
using namespace cv;

const static int VIEW_MODE_FAST = 1;
const static int VIEW_MODE_ORB = 2;
const static int VIEW_MODE_STAR = 3;
const static int VIEW_MODE_MSER = 4;

FastFeatureDetector fastd(8/*response_threshold*/, true/*non_max_suppression*/);
OrbFeatureDetector  orbd (200/*nfeatures*/, 1.2f/*scaleFactor*/, 8/*nlevels*/);
StarFeatureDetector stard(20/*max_size*/, 8/*response_threshold*/, 15/*line_threshold_projected*/, 8/*line_threshold_binarized*/, 5/*suppress_nonmax_size*/);
MserFeatureDetector mserd(10/*delta*/, 10/*min_area*/, 500/*max_area*/, 0.2/*max_variation*/, 0.7/*min_diversity*/, 2/*max_evolution*/, 1/*area_threshold*/, 5/*min_margin*/, 2/*edge_blur_size*/);

extern "C" {
JNIEXPORT void JNICALL Java_edu_stanford_ee368_featurekeypoints_FeatureKeypointsActivity_NativeFAST(JNIEnv*, jobject, jlong addrGray, jlong addrRgba, jint featureType);

JNIEXPORT void JNICALL Java_edu_stanford_ee368_featurekeypoints_FeatureKeypointsActivity_NativeProcessing(JNIEnv*, jobject, jlong addrGray, jlong addrRgba, jint featureType) {
    Mat& matGray  = *(Mat*)addrGray;
    Mat& matRgb = *(Mat*)addrRgba;

    // Pick the correct feature detector
    FeatureDetector* fd = 0;
    switch (featureType) {
        case VIEW_MODE_FAST:
            fd = &fastd;
            break;
        case VIEW_MODE_ORB:
            fd = &orbd;
            break;
        case VIEW_MODE_STAR:
            fd = &stard;
            break;
        case VIEW_MODE_MSER:
            fd = &mserd;
            break;
    }

    // Define the keypoints vector
    std::vector<cv::KeyPoint> keypoints;

    // Detect new keypoints
    fd->detect(matGray, keypoints);

    // Draw the keypoints on the images
    for (vector<KeyPoint>::const_iterator it = keypoints.begin(); it != keypoints.end(); ++it) {
        // Draw black circles, slightly offset
        Point2f pt = it->pt;
        pt.x -= 2;
        pt.y -= 2;
        if (featureType == VIEW_MODE_FAST)
            circle(matRgb, pt, it->size, cvScalar(0, 0, 0, 0), 2);
        else if (featureType == VIEW_MODE_ORB)
            circle(matRgb, pt, it->size*0.2, cvScalar(0, 0, 0, 0), 2);
        else if (featureType == VIEW_MODE_STAR)
            circle(matRgb, pt, 5, cvScalar(0, 0, 0, 0), 2);
        else
            circle(matRgb, pt, it->size, cvScalar(0, 0, 0, 0), 2);

        // Draw yellow circles
        pt.x += 2;
        pt.y += 2;
        if (featureType == VIEW_MODE_FAST)
            circle(matRgb, pt, it->size, cvScalar(255, 255, 0, 0), 2);
        else if (featureType == VIEW_MODE_ORB)
            circle(matRgb, pt, it->size*0.2, cvScalar(255, 255, 0, 0), 2);
        else if (featureType == VIEW_MODE_STAR)
            circle(matRgb, pt, 5, cvScalar(255, 255, 0, 0), 2);
        else
            circle(matRgb, pt, it->size, cvScalar(255, 255, 0, 0), 2);
    }
}
}
