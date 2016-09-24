package edu.stanford.ee368.featurekeypoints;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.WindowManager;
import android.widget.Toast;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.CvType;
import org.opencv.core.Mat;

public class FeatureKeypointsActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
    private static final String    TAG = "FeatKeypointsActivity";

    private static final int       VIEW_MODE_RGBA = 0;
    private static final int       VIEW_MODE_FAST = 1;
    private static final int       VIEW_MODE_ORB = 2;
    private static final int       VIEW_MODE_STAR = 3;
    private static final int       VIEW_MODE_MSER = 4;

    private static final int       DIALOG_OPENING_TUTORIAL = 0;
    private static final int       DIALOG_TUTORIAL_FAST = 1;
    private static final int       DIALOG_TUTORIAL_ORB = 2;
    private static final int       DIALOG_TUTORIAL_STAR = 3;
    private static final int       DIALOG_TUTORIAL_MSER = 4;

    private int                    mViewMode;
    private Mat mRgba;
    private Mat                    mGray;

    private MenuItem               mItemFast;
    private MenuItem               mItemOrb;
    private MenuItem               mItemStar;
    private MenuItem               mItemMser;

    private CameraBridgeViewBase   mOpenCvCameraView;

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i(TAG, "OpenCV loaded successfully");

                    // Load native library after OpenCV initialization
                    System.loadLibrary("native-lib");

                    mOpenCvCameraView.enableView();
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    public FeatureKeypointsActivity() {
        Log.i(TAG, "Instantiated new " + this.getClass());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "called onCreate");
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_feature_keypoints);

        mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.surface_view);
        mOpenCvCameraView.setMaxFrameSize(640, 480);
        mOpenCvCameraView.setCvCameraViewListener(this);

        mViewMode = VIEW_MODE_RGBA;

        // Set opening toast
        toasts(DIALOG_OPENING_TUTORIAL);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        Log.i(TAG, "called onCreateOptionsMenu");
        mItemFast = menu.add("FAST");
        mItemOrb = menu.add("ORB");
        mItemStar = menu.add("STAR");
        mItemMser = menu.add("MSER");
        return true;
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    @Override
    public void onResume() {
        super.onResume();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_3, this, mLoaderCallback);
    }

    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    public void onCameraViewStarted(int width, int height) {
        mRgba = new Mat(height, width, CvType.CV_8UC4);
        mGray = new Mat(height, width, CvType.CV_8UC1);
    }

    public void onCameraViewStopped() {
        mRgba.release();
        mGray.release();
    }

    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        final int viewMode = mViewMode;
        mRgba = inputFrame.rgba();
        mGray = inputFrame.gray();
        switch (viewMode) {
            case VIEW_MODE_RGBA:
                break;
            case VIEW_MODE_FAST:
                NativeProcessing(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr(), VIEW_MODE_FAST);
                break;
            case VIEW_MODE_ORB:
                NativeProcessing(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr(), VIEW_MODE_ORB);
                break;
            case VIEW_MODE_STAR:
                NativeProcessing(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr(), VIEW_MODE_STAR);
                break;
            case VIEW_MODE_MSER:
                NativeProcessing(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr(), VIEW_MODE_MSER);
                break;
        }

        return mRgba;
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        Log.i(TAG, "called onOptionsItemSelected; selected item: " + item);

        if (item == mItemFast) {
            mViewMode = VIEW_MODE_FAST;
            toasts(DIALOG_TUTORIAL_FAST);
        } else if (item == mItemOrb) {
            mViewMode = VIEW_MODE_ORB;
            toasts(DIALOG_TUTORIAL_ORB);
        } else if (item == mItemStar) {
            mViewMode = VIEW_MODE_STAR;
            toasts(DIALOG_TUTORIAL_STAR);
        } else if (item == mItemMser) {
            mViewMode = VIEW_MODE_MSER;
            toasts(DIALOG_TUTORIAL_MSER);
        }

        return true;
    }

    void toasts(int id) {
        switch (id) {
            case DIALOG_OPENING_TUTORIAL:
                Toast.makeText(this, "Try clicking the menu for CV options.",
                        Toast.LENGTH_LONG).show();
                break;
            case DIALOG_TUTORIAL_FAST:
                Toast.makeText(this, "Detecting and displaying FAST keypoints",
                        Toast.LENGTH_LONG).show();
                break;
            case DIALOG_TUTORIAL_ORB:
                Toast.makeText(this, "Detecting and displaying ORB keypoints",
                        Toast.LENGTH_LONG).show();
                break;
            case DIALOG_TUTORIAL_STAR:
                Toast.makeText(this, "Detecting and displaying STAR keypoints",
                        Toast.LENGTH_LONG).show();
                break;
            case DIALOG_TUTORIAL_MSER:
                Toast.makeText(this, "Detecting and displaying MSER keypoints",
                        Toast.LENGTH_LONG).show();
                break;
            default:
                break;
        }
    }

    public native void NativeProcessing(long matAddrGr, long matAddrRgba, int featureType);
}