package net.openwatch.openwatch2.video;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.SurfaceView;

public class VideoSoftwareRecorder {

	private static final String TAG = "VideoSoftwareRecorder";
	public static Camera camera;
	
	public static boolean is_recording = false;
	
	private static FFEncoder ffencoder;

	public static void startRecording(SurfaceView camera_surface_view,
			File output_file) {
		
		ffencoder = new FFEncoder();
		ffencoder.initializeEncoder(getFilePath(output_file), 320, 240);

		if (camera == null)
			camera = Camera.open();
		else
			return; // The last video recording was not stopped properly
		
		Camera.Parameters camera_parameters = camera.getParameters();
		camera_parameters.setPreviewFormat(ImageFormat.NV21);
		// I want 420 YpCbCr 8 biplanar full range
		camera_parameters.setPreviewSize(320, 240);
		camera.setParameters(camera_parameters);

		try {
			camera.setPreviewDisplay(camera_surface_view.getHolder());
		} catch (IOException e) {
			Log.e(TAG, "setPreviewDisplay IOE");
			e.printStackTrace();
		}
		
		Size previewSize = camera.getParameters().getPreviewSize();
		int dataBufferSize=(int)(previewSize.height*previewSize.width*
                (ImageFormat.getBitsPerPixel(camera.getParameters().getPreviewFormat())/8.0));
		
		camera.setPreviewCallback(new Camera.PreviewCallback() {
			
			@Override
			public void onPreviewFrame(byte[] data, Camera camera) {
				//Log.d(TAG,"Frame received");
				ffencoder.encodeFrame(data);
			}
		});
		
		camera.startPreview();
		is_recording = true;
	}

	public static void stopRecording() {
		camera.stopPreview();
		camera.setPreviewCallback(null);
		ffencoder.finalizeEncoder();
		camera.release();
		camera = null;
		is_recording = false;
	}
	
	public static String getFilePath(File output_file){
		if(!output_file.exists()){
			try {
				output_file.createNewFile();
			} catch (IOException e) {
				Log.e(TAG, "New File IOE");
				e.printStackTrace();
			}
		}
		return output_file.getAbsolutePath();

	}

}
