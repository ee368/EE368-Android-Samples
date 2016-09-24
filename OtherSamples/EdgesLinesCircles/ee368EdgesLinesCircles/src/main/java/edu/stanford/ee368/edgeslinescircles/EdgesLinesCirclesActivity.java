package edu.stanford.ee368.edgeslinescircles;

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

public class EdgesLinesCirclesActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
    private static final String    TAG = "ELCActivity";

    private static final int       VIEW_MODE_RGBA = 0;
    private static final int       VIEW_MODE_CANNY = 1;
    private static final int       VIEW_MODE_HOUGH_LINES = 2;
    private static final int       VIEW_MODE_HOUGH_CIRCLES = 3;

    private static final int       DIALOG_OPENING_TUTORIAL = 0;
    private static final int       DIALOG_TUTORIAL_CANNY = 1;
    private static final int       DIALOG_TUTORIAL_HOUGH_LINES = 2;
    private static final int       DIALOG_TUTORIAL_HOUGH_CIRCLES = 3;

    private int                    mViewMode;
    private Mat                    mRgba;
    private Mat                    mGray;

    private MenuItem               mItemCanny;
    private MenuItem               mItemHoughLines;
    private MenuItem               mItemHoughCircles;

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

    public EdgesLinesCirclesActivity() {
        Log.i(TAG, "Instantiated new " + this.getClass());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "called onCreate");
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_edges_lines_circles);

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
        mItemCanny = menu.add("Canny");
        mItemHoughLines = menu.add("Hough Lines");
        mItemHoughCircles = menu.add("Hough Circles");
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
            case VIEW_MODE_CANNY:
                NativeCanny(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr());
                break;
            case VIEW_MODE_HOUGH_LINES:
                NativeHoughLines(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr());
                break;
            case VIEW_MODE_HOUGH_CIRCLES:
                NativeHoughCircles(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr());
                break;
        }

        return mRgba;
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        Log.i(TAG, "called onOptionsItemSelected; selected item: " + item);

        if (item == mItemCanny) {
            mViewMode = VIEW_MODE_CANNY;
            toasts(DIALOG_TUTORIAL_CANNY);
        } else if (item == mItemHoughLines) {
            mViewMode = VIEW_MODE_HOUGH_LINES;
            toasts(DIALOG_TUTORIAL_HOUGH_LINES);
        } else if (item == mItemHoughCircles) {
            mViewMode = VIEW_MODE_HOUGH_CIRCLES;
            toasts(DIALOG_TUTORIAL_HOUGH_CIRCLES);
        }

        return true;
    }

    void toasts(int id) {
        switch (id) {
            case DIALOG_OPENING_TUTORIAL:
                Toast.makeText(this, "Try clicking the menu for CV options.",
                        Toast.LENGTH_LONG).show();
                break;
            case DIALOG_TUTORIAL_CANNY:
                Toast.makeText(this, "Detecting and displaying Canny edges",
                        Toast.LENGTH_LONG).show();
                break;
            case DIALOG_TUTORIAL_HOUGH_LINES:
                Toast.makeText(this, "Detecting and displaying strong lines",
                        Toast.LENGTH_LONG).show();
                break;
            case DIALOG_TUTORIAL_HOUGH_CIRCLES:
                Toast.makeText(this, "Detecting and displaying strong circles",
                        Toast.LENGTH_LONG).show();
                break;
            default:
                break;
        }
    }

    public native void NativeCanny(long matAddrGr, long matAddrRgba);
    public native void NativeHoughLines(long matAddrGr, long matAddrRgba);
    public native void NativeHoughCircles(long matAddrGr, long matAddrRgba);
}