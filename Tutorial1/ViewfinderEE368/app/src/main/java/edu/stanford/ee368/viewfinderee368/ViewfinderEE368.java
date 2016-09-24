package edu.stanford.ee368.viewfinderee368;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

import java.io.IOException;

// ----------------------------------------------------------------------

public class ViewfinderEE368 extends Activity {
    private Preview mPreview;
    private DrawOnTop mDrawOnTop;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Hide the window title.
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        // Request camera permissions if needed
        // (user input is required for granting permissions in Android 6.0 and later)
        int permissionCheck = ContextCompat.checkSelfPermission(this,
                Manifest.permission.CAMERA);
        if(permissionCheck != PackageManager.PERMISSION_GRANTED){
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.CAMERA},
                    0);
        } else {
            createPreview();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        // If request is cancelled, the result arrays are empty.
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            // Permission granted
            createPreview();
        } else {
            // Permission denied
            finish();
        }
        return;
    }

    private void createPreview(){
        // Create our Preview view and set it as the content of our activity.
        // Create our DrawOnTop view.
        mDrawOnTop = new DrawOnTop(this);
        mPreview = new Preview(this, mDrawOnTop);
        setContentView(mPreview);
        addContentView(mDrawOnTop, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    }
}

//----------------------------------------------------------------------

class DrawOnTop extends View {
    Paint mPaintBlack;
    Paint mPaintYellow;
    Paint mPaintRed;
    Paint mPaintGreen;
    Paint mPaintBlue;
    byte[] mYUVData;
    int[] mRGBData;
    int mImageWidth, mImageHeight;
    int[] mRedHistogram;
    int[] mGreenHistogram;
    int[] mBlueHistogram;
    double[] mBinSquared;

    public DrawOnTop(Context context) {
        super(context);

        mPaintBlack = new Paint();
        mPaintBlack.setStyle(Paint.Style.FILL);
        mPaintBlack.setColor(Color.BLACK);
        mPaintBlack.setTextSize(25);

        mPaintYellow = new Paint();
        mPaintYellow.setStyle(Paint.Style.FILL);
        mPaintYellow.setColor(Color.YELLOW);
        mPaintYellow.setTextSize(25);

        mPaintRed = new Paint();
        mPaintRed.setStyle(Paint.Style.FILL);
        mPaintRed.setColor(Color.RED);
        mPaintRed.setTextSize(25);

        mPaintGreen = new Paint();
        mPaintGreen.setStyle(Paint.Style.FILL);
        mPaintGreen.setColor(Color.GREEN);
        mPaintGreen.setTextSize(25);

        mPaintBlue = new Paint();
        mPaintBlue.setStyle(Paint.Style.FILL);
        mPaintBlue.setColor(Color.BLUE);
        mPaintBlue.setTextSize(25);

        mYUVData = null;
        mRGBData = null;
        mRedHistogram = new int[256];
        mGreenHistogram = new int[256];
        mBlueHistogram = new int[256];
        mBinSquared = new double[256];
        for (int bin = 0; bin < 256; bin++)
        {
            mBinSquared[bin] = ((double)bin) * bin;
        } // bin
    }

    @Override
    protected void onDraw(Canvas canvas) {
        int canvasWidth = canvas.getWidth();
        int canvasHeight = canvas.getHeight();

        // Convert from YUV to RGB
        decodeYUV420SP(mRGBData, mYUVData, mImageWidth, mImageHeight);

        // Calculate histogram
        calculateIntensityHistogram(mRGBData, mRedHistogram,
                mImageWidth, mImageHeight, 0);
        calculateIntensityHistogram(mRGBData, mGreenHistogram,
                mImageWidth, mImageHeight, 1);
        calculateIntensityHistogram(mRGBData, mBlueHistogram,
                mImageWidth, mImageHeight, 2);

        // Calculate mean
        double imageRedMean = 0, imageGreenMean = 0, imageBlueMean = 0;
        double redHistogramSum = 0, greenHistogramSum = 0, blueHistogramSum = 0;
        for (int bin = 0; bin < 256; bin++)
        {
            imageRedMean += mRedHistogram[bin] * bin;
            redHistogramSum += mRedHistogram[bin];
            imageGreenMean += mGreenHistogram[bin] * bin;
            greenHistogramSum += mGreenHistogram[bin];
            imageBlueMean += mBlueHistogram[bin] * bin;
            blueHistogramSum += mBlueHistogram[bin];
        } // bin
        imageRedMean /= redHistogramSum;
        imageGreenMean /= greenHistogramSum;
        imageBlueMean /= blueHistogramSum;

        // Calculate second moment
        double imageRed2ndMoment = 0, imageGreen2ndMoment = 0, imageBlue2ndMoment = 0;
        for (int bin = 0; bin < 256; bin++)
        {
            imageRed2ndMoment += mRedHistogram[bin] * mBinSquared[bin];
            imageGreen2ndMoment += mGreenHistogram[bin] * mBinSquared[bin];
            imageBlue2ndMoment += mBlueHistogram[bin] * mBinSquared[bin];
        } // bin
        imageRed2ndMoment /= redHistogramSum;
        imageGreen2ndMoment /= greenHistogramSum;
        imageBlue2ndMoment /= blueHistogramSum;
        double imageRedStdDev = Math.sqrt( imageRed2ndMoment - imageRedMean*imageRedMean );
        double imageGreenStdDev = Math.sqrt( imageGreen2ndMoment - imageGreenMean*imageGreenMean );
        double imageBlueStdDev = Math.sqrt( imageBlue2ndMoment - imageBlueMean*imageBlueMean );

        // Draw mean
        String imageMeanStr = "Mean (R,G,B): " + String.format("%.4g", imageRedMean) + ", " + String.format("%.4g", imageGreenMean) + ", " + String.format("%.4g", imageBlueMean);
        canvas.drawText(imageMeanStr, 10-1, 30-1, mPaintBlack);
        canvas.drawText(imageMeanStr, 10+1, 30-1, mPaintBlack);
        canvas.drawText(imageMeanStr, 10+1, 30+1, mPaintBlack);
        canvas.drawText(imageMeanStr, 10-1, 30+1, mPaintBlack);
        canvas.drawText(imageMeanStr, 10, 30, mPaintYellow);

        // Draw standard deviation
        String imageStdDevStr = "Std Dev (R,G,B): " + String.format("%.4g", imageRedStdDev) + ", " + String.format("%.4g", imageGreenStdDev) + ", " + String.format("%.4g", imageBlueStdDev);
        canvas.drawText(imageStdDevStr, 10-1, 60-1, mPaintBlack);
        canvas.drawText(imageStdDevStr, 10+1, 60-1, mPaintBlack);
        canvas.drawText(imageStdDevStr, 10+1, 60+1, mPaintBlack);
        canvas.drawText(imageStdDevStr, 10-1, 60+1, mPaintBlack);
        canvas.drawText(imageStdDevStr, 10, 60, mPaintYellow);

        // Draw red intensity histogram
        float barMaxHeight = 3000;
        float barWidth = ((float)canvasWidth) / 256;
        float barMarginHeight = 2;
        RectF barRect = new RectF();
        barRect.bottom = canvasHeight - 200;
        barRect.left = 0;
        barRect.right = barRect.left + barWidth;
        for (int bin = 0; bin < 256; bin++)
        {
            float prob = (float)mRedHistogram[bin] / (float)redHistogramSum;
            barRect.top = barRect.bottom -
                    Math.min(80,prob*barMaxHeight) - barMarginHeight;
            canvas.drawRect(barRect, mPaintBlack);
            barRect.top += barMarginHeight;
            canvas.drawRect(barRect, mPaintRed);
            barRect.left += barWidth;
            barRect.right += barWidth;
        } // bin

        // Draw green intensity histogram
        barRect.bottom = canvasHeight - 100;
        barRect.left = 0;
        barRect.right = barRect.left + barWidth;
        for (int bin = 0; bin < 256; bin++)
        {
            barRect.top = barRect.bottom - Math.min(80, ((float)mGreenHistogram[bin])/((float)greenHistogramSum) * barMaxHeight) - barMarginHeight;
            canvas.drawRect(barRect, mPaintBlack);
            barRect.top += barMarginHeight;
            canvas.drawRect(barRect, mPaintGreen);
            barRect.left += barWidth;
            barRect.right += barWidth;
        } // bin

        // Draw blue intensity histogram
        barRect.bottom = canvasHeight;
        barRect.left = 0;
        barRect.right = barRect.left + barWidth;
        for (int bin = 0; bin < 256; bin++)
        {
            barRect.top = barRect.bottom - Math.min(80, ((float)mBlueHistogram[bin])/((float)blueHistogramSum) * barMaxHeight) - barMarginHeight;
            canvas.drawRect(barRect, mPaintBlack);
            barRect.top += barMarginHeight;
            canvas.drawRect(barRect, mPaintBlue);
            barRect.left += barWidth;
            barRect.right += barWidth;
        } // bin

        super.onDraw(canvas);

    } // end onDraw method

    static public void decodeYUV420SP(int[] rgb, byte[] yuv420sp, int width, int height) {
        final int frameSize = width * height;

        for (int j = 0, yp = 0; j < height; j++) {
            int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
            for (int i = 0; i < width; i++, yp++) {
                int y = (0xff & ((int) yuv420sp[yp])) - 16;
                if (y < 0) y = 0;
                if ((i & 1) == 0) {
                    v = (0xff & yuv420sp[uvp++]) - 128;
                    u = (0xff & yuv420sp[uvp++]) - 128;
                }

                int y1192 = 1192 * y;
                int r = (y1192 + 1634 * v);
                int g = (y1192 - 833 * v - 400 * u);
                int b = (y1192 + 2066 * u);

                if (r < 0) r = 0; else if (r > 262143) r = 262143;
                if (g < 0) g = 0; else if (g > 262143) g = 262143;
                if (b < 0) b = 0; else if (b > 262143) b = 262143;

                rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
            }
        }
    }

    static public void decodeYUV420SPGrayscale(int[] rgb, byte[] yuv420sp, int width, int height)
    {
        final int frameSize = width * height;

        for (int pix = 0; pix < frameSize; pix++)
        {
            int pixVal = (0xff & ((int) yuv420sp[pix])) - 16;
            if (pixVal < 0) pixVal = 0;
            if (pixVal > 255) pixVal = 255;
            rgb[pix] = 0xff000000 | (pixVal << 16) | (pixVal << 8) | pixVal;
        } // pix
    }

    static public void calculateIntensityHistogram(int[] rgb, int[] histogram, int width, int height, int component)
    {
        for (int bin = 0; bin < 256; bin++)
        {
            histogram[bin] = 0;
        } // bin
        if (component == 0) // red
        {
            for (int pix = 0; pix < width*height; pix += 3)
            {
                int pixVal = (rgb[pix] >> 16) & 0xff;
                histogram[ pixVal ]++;
            } // pix
        }
        else if (component == 1) // green
        {
            for (int pix = 0; pix < width*height; pix += 3)
            {
                int pixVal = (rgb[pix] >> 8) & 0xff;
                histogram[ pixVal ]++;
            } // pix
        }
        else // blue
        {
            for (int pix = 0; pix < width*height; pix += 3)
            {
                int pixVal = rgb[pix] & 0xff;
                histogram[ pixVal ]++;
            } // pix
        }
    }
}

// ----------------------------------------------------------------------

class Preview extends SurfaceView implements SurfaceHolder.Callback {
    SurfaceHolder mHolder;
    Camera mCamera;
    DrawOnTop mDrawOnTop;
    boolean mFinished;

    Preview(Context context, DrawOnTop drawOnTop) {
        super(context);

        mDrawOnTop = drawOnTop;
        mFinished = false;

        // Install a SurfaceHolder.Callback so we get notified when the
        // underlying surface is created and destroyed.
        mHolder = getHolder();
        mHolder.addCallback(this);
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    public void surfaceCreated(SurfaceHolder holder) {
        mCamera = Camera.open();
        try {
            mCamera.setPreviewDisplay(holder);

            // Preview callback used whenever new viewfinder frame is available
            mCamera.setPreviewCallback(new PreviewCallback() {
                public void onPreviewFrame(byte[] data, Camera camera)
                {
                    if ( (mDrawOnTop == null) || mFinished )
                        return;

                    if (mDrawOnTop.mRGBData == null)
                    {
                        // Initialize the draw-on-top companion
                        Camera.Parameters params = camera.getParameters();
                        mDrawOnTop.mImageWidth = params.getPreviewSize().width;
                        mDrawOnTop.mImageHeight = params.getPreviewSize().height;
                        mDrawOnTop.mRGBData = new int[mDrawOnTop.mImageWidth * mDrawOnTop.mImageHeight];
                        mDrawOnTop.mYUVData = new byte[data.length];
                    }

                    // Pass YUV data to draw-on-top companion
                    System.arraycopy(data, 0, mDrawOnTop.mYUVData, 0, data.length);
                    mDrawOnTop.invalidate();
                }
            });
        }
        catch (IOException exception) {
            mCamera.release();
            mCamera = null;
        }
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        // Surface will be destroyed when we return, so stop the preview.
        // Because the CameraDevice object is not a shared resource, it's very
        // important to release it when the activity is paused.
        mFinished = true;
        mCamera.setPreviewCallback(null);
        mCamera.stopPreview();
        mCamera.release();
        mCamera = null;
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        // Now that the size is known, set up the camera parameters and begin
        // the preview.
        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewSize(320, 240);
        parameters.setPreviewFrameRate(15);
        parameters.setSceneMode(Camera.Parameters.SCENE_MODE_NIGHT);
        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
        mCamera.setParameters(parameters);
        mCamera.startPreview();
    }

}