package com.ee368.siftexample;

//********************************************************************************************
//EE368 Digital Image Processing
//Android Tutorial #3: Server-Client Communication
//Author: Derek Pang (dcypang@stanford.edu), David Chen (dmchen@stanford.edu)
//********************************************************************************************/

import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;


import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.ShutterCallback;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.KeyEvent;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.WindowManager;
import android.os.Bundle;

public class SIFTExampleActivity extends Activity {

	private static final String TAG = "SIFTExampleActivity";

	Preview mPreview;
	ResultView mResultView;
	private Context mContext = this;

	/** PLEASE PUT YOUR SERVER URL **/
	private final String SERVERURL = "";

	private final static String INPUT_IMG_FILENAME = "/temp.jpg"; //name for storing image captured by camera view

	//flag to check if camera is ready for capture
	private boolean mCameraReadyFlag = true;

	// Called when the activity is first created.
	@Override
	public void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);

		//make the screen full screen
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		//remove the title bar
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		mResultView=new ResultView(this);
		mPreview = new Preview(this);

		//set Content View as the preview
		setContentView(mPreview);

		//add result view  to the content View
		addContentView(mResultView,new LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT));

		//set the orientation as landscape
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
	}

	// Called when shutter is opened
	ShutterCallback shutterCallback = new ShutterCallback() {
		public void onShutter() {
		}
	};

	// Handles data for raw picture
	PictureCallback rawCallback = new PictureCallback() {
		@Override
		public void onPictureTaken(byte[] arg0, android.hardware.Camera arg1) {
		}
	};

	//store the image as a jpeg image
	public  boolean compressByteImage(Context mContext, byte[] imageData,
									  int quality) {
		File sdCard = Environment.getExternalStorageDirectory();
		FileOutputStream fileOutputStream = null;

		try {
			BitmapFactory.Options options=new BitmapFactory.Options();
			options.inSampleSize = 1;  	//no downsampling
			Bitmap myImage = BitmapFactory.decodeByteArray(imageData, 0,
					imageData.length,options);
			fileOutputStream = new FileOutputStream(
					sdCard.toString() +INPUT_IMG_FILENAME);

			BufferedOutputStream bos = new BufferedOutputStream(
					fileOutputStream);

			//compress image to jpeg
			myImage.compress(CompressFormat.JPEG, quality, bos);

			bos.flush();
			bos.close();
			fileOutputStream.close();

		} catch (FileNotFoundException e) {
			Log.e(TAG, "FileNotFoundException");
			e.printStackTrace();
		} catch (IOException e) {
			Log.e(TAG, "IOException");
			e.printStackTrace();
		}
		return true;
	}

	// Handles data for jpeg picture
	PictureCallback jpegCallback = new PictureCallback() {
		@Override
		public void onPictureTaken(byte[] imageData, android.hardware.Camera camera) {
			if (imageData != null) {

				Intent mIntent = new Intent();
				//compress image
				compressByteImage(mContext, imageData, 75);
				setResult(0, mIntent);

				//** Send image and offload image processing task  to server by starting async task **
				ServerTask task = new ServerTask();
				task.execute( Environment.getExternalStorageDirectory().toString() +INPUT_IMG_FILENAME);

				//start the camera view again .
				camera.startPreview();
			}
		}
	};

	//*******************************************************************************
	//UI
	//*******************************************************************************

	//onKeyDown is used to monitor button pressed and facilitate the switching of views
	@Override
	public boolean onKeyDown(int keycode,KeyEvent event)
	{
		//check if the camera button is pressed
		if(keycode==KeyEvent.KEYCODE_VOLUME_UP)
		{
			//if result
			if (mResultView.IsShowingResult)
			{
				mResultView.IsShowingResult = false;
			}
			else if (mCameraReadyFlag == true)//switch to camera view
			{
				mCameraReadyFlag = false;
				mPreview.camera.takePicture(shutterCallback, rawCallback, jpegCallback);
			}
			return true;
		}
		return super.onKeyDown(keycode, event);
	}

	//*******************************************************************************
	//Push image processing task to server
	//*******************************************************************************

	public class ServerTask  extends AsyncTask<String, Integer , Void>
	{
		public byte[] dataToServer;

		//Task state
		private final int UPLOADING_PHOTO_STATE  = 0;
		private final int SERVER_PROC_STATE  = 1;

		private ProgressDialog dialog;

		//upload photo to server
		HttpURLConnection uploadPhoto(FileInputStream fileInputStream)
		{

			final String serverFileName = "test"+ (int) Math.round(Math.random()*1000) + ".jpg";
			final String lineEnd = "\r\n";
			final String twoHyphens = "--";
			final String boundary = "*****";

			try
			{
				URL url = new URL(SERVERURL);
				// Open a HTTP connection to the URL
				final HttpURLConnection conn = (HttpURLConnection)url.openConnection();
				// Allow Inputs
				conn.setDoInput(true);
				// Allow Outputs
				conn.setDoOutput(true);
				// Don't use a cached copy.
				conn.setUseCaches(false);

				// Use a post method.
				conn.setRequestMethod("POST");
				conn.setRequestProperty("Connection", "Keep-Alive");
				conn.setRequestProperty("Content-Type", "multipart/form-data;boundary="+boundary);

				DataOutputStream dos = new DataOutputStream( conn.getOutputStream() );

				dos.writeBytes(twoHyphens + boundary + lineEnd);
				dos.writeBytes("Content-Disposition: form-data; name=\"uploadedfile\";filename=\"" + serverFileName +"\"" + lineEnd);
				dos.writeBytes(lineEnd);

				// create a buffer of maximum size
				int bytesAvailable = fileInputStream.available();
				int maxBufferSize = 1024;
				int bufferSize = Math.min(bytesAvailable, maxBufferSize);
				byte[] buffer = new byte[bufferSize];

				// read file and write it into form...
				int bytesRead = fileInputStream.read(buffer, 0, bufferSize);

				while (bytesRead > 0)
				{
					dos.write(buffer, 0, bufferSize);
					bytesAvailable = fileInputStream.available();
					bufferSize = Math.min(bytesAvailable, maxBufferSize);
					bytesRead = fileInputStream.read(buffer, 0, bufferSize);
				}

				// send multipart form data after file data...
				dos.writeBytes(lineEnd);
				dos.writeBytes(twoHyphens + boundary + twoHyphens + lineEnd);
				publishProgress(SERVER_PROC_STATE);
				// close streams
				fileInputStream.close();
				dos.flush();

				return conn;
			}
			catch (MalformedURLException ex){
				Log.e(TAG, "error: " + ex.getMessage(), ex);
				return null;
			}
			catch (IOException ioe){
				Log.e(TAG, "error: " + ioe.getMessage(), ioe);
				return null;
			}
		}

		//get image result from server and display it in result view
		void getResultImage(HttpURLConnection conn){
			// retrieve the response from server
			InputStream is;
			try {
				is = conn.getInputStream();
				//get result image from server
				mResultView.resultImage = BitmapFactory.decodeStream(is);
				is.close();
				mResultView.IsShowingResult = true;
			} catch (IOException e) {
				Log.e(TAG,e.toString());
				e.printStackTrace();
			}
		}

		//Main code for processing image algorithm on the server

		void processImage(String inputImageFilePath){
			publishProgress(UPLOADING_PHOTO_STATE);
			File inputFile = new File(inputImageFilePath);
			try {

				//create file stream for captured image file
				FileInputStream fileInputStream  = new FileInputStream(inputFile);

				//upload photo
				final HttpURLConnection  conn = uploadPhoto(fileInputStream);

				//get processed photo from server
				if (conn != null){
					getResultImage(conn);}
				fileInputStream.close();
			}
			catch (FileNotFoundException ex){
				Log.e(TAG, ex.toString());
			}
			catch (IOException ex){
				Log.e(TAG, ex.toString());
			}
		}

		public ServerTask() {
			dialog = new ProgressDialog(mContext);
		}

		protected void onPreExecute() {
			this.dialog.setMessage("Photo captured");
			this.dialog.show();
		}
		@Override
		protected Void doInBackground(String... params) {			//background operation 
			String uploadFilePath = params[0];
			processImage(uploadFilePath);
			//release camera when previous image is processed
			mCameraReadyFlag = true;
			return null;
		}
		//progress update, display dialogs
		@Override
		protected void onProgressUpdate(Integer... progress) {
			if(progress[0] == UPLOADING_PHOTO_STATE){
				dialog.setMessage("Uploading");
				dialog.setCanceledOnTouchOutside(false);
				dialog.show();
			}
			else if (progress[0] == SERVER_PROC_STATE){
				if (dialog.isShowing()) {
					dialog.dismiss();
				}
				dialog.setMessage("Processing");
				dialog.setCanceledOnTouchOutside(false);
				dialog.show();
			}
		}
		@Override
		protected void onPostExecute(Void param) {
			if (dialog.isShowing()) {
				dialog.dismiss();
			}
		}
	}
}