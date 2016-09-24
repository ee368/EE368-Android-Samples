package edu.stanford.ee368.featuretracking;

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

public class FeatureTrackingActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
    private static final String    TAG = "FeatureTrackingActivity";

    private static final int       VIEW_MODE_RGBA = 0;
    private static final int       VIEW_MODE_TRACKING = 1;

    private static final int       DIALOG_OPENING_TUTORIAL = 0;
    private static final int       DIALOG_TUTORIAL_TRACKING = 1;

    private int                    mViewMode;
    private Mat                    mRgba;
    private Mat                    mGray;

    private MenuItem               mItemTracking;

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

    public FeatureTrackingActivity() {
        Log.i(TAG, "Instantiated new " + this.getClass());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "called onCreate");
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_feature_tracking);

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
        mItemTracking = menu.add("Tracking");
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
            case VIEW_MODE_TRACKING:
                NativeProcessing(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr());
                break;
        }

        return mRgba;
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        Log.i(TAG, "called onOptionsItemSelected; selected item: " + item);

        if (item == mItemTracking) {
            mViewMode = VIEW_MODE_TRACKING;
            toasts(DIALOG_TUTORIAL_TRACKING);
        }

        return true;
    }

    void toasts(int id) {
        switch (id) {
            case DIALOG_OPENING_TUTORIAL:
                Toast.makeText(this, "Try clicking the menu for CV options.",
                        Toast.LENGTH_LONG).show();
                break;
            case DIALOG_TUTORIAL_TRACKING:
                Toast.makeText(this, "Tracking motion in viewfinder frames",
                        Toast.LENGTH_LONG).show();
                break;
            default:
                break;
        }
    }

    public native void NativeProcessing(long matAddrGr, long matAddrRgba);
}