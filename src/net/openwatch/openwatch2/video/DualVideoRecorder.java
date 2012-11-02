package net.openwatch.openwatch2.video;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Date;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.media.MediaRecorder;
import android.util.Log;
import android.view.SurfaceView;

public class DualVideoRecorder {
	private static final String TAG = "DualVideoRecorder";
	
	public static Camera camera; 
	
	public static boolean is_recording = false;
		
	private static FFDualVideoEncoder ffencoder;
	
	/** 
	 * Begin recording video the the output_file specified.
	 * @param camera_surface_view the SurfaceView to attach the camera preview to
	 * @param output_file the destination of the recording. This file will be created if it doesn't
	 * all ready exist.
	 */
	public static void startRecording(SurfaceView camera_surface_view, String file_path){
		
		ffencoder = new FFDualVideoEncoder();
		
		
		String file_name = String.valueOf(new Date().getTime());
		//ffencoder.initializeEncoder(getFilePath(new File(file_path + file_name + "_LQ.mpg")), 320, 240);
		
		//camera_output_stream = getOutputStreamFromFile(output_file);

		if(camera == null)
			camera = Camera.open();
		
		// TODO: Camera setup method: autofocus, setRecordingHint etc.
		Camera.Parameters camera_parameters = camera.getParameters();
		camera_parameters.setPreviewFormat(ImageFormat.NV21);
		camera_parameters.setPreviewSize(320, 240);
		//camera_parameters.setRecordingHint(true);
		camera.setParameters(camera_parameters);
		
		try {
			camera.setPreviewDisplay(camera_surface_view.getHolder());
		} catch (IOException e) {
			Log.e(TAG, "setPreviewDisplay IOE");
			e.printStackTrace();
		}
		
		camera.setPreviewCallback(new Camera.PreviewCallback() {
			
			@Override
			public void onPreviewFrame(byte[] data, Camera camera) {
				hq_ffencoder.encodeFrame(data);
				lq_ffencoder.encodeFrame(data);
				//Log.d(TAG,"preview frame got");
				
			}
		});
		
		camera.startPreview();
		//camera.unlock();

		is_recording = true;

	}
	
	public static void stopRecording(){
		camera.stopPreview();
		camera.setPreviewCallback(null);
		hq_ffencoder.finalizeEncoder();
		lq_ffencoder.finalizeEncoder();
		camera.release();
		camera = null;
		is_recording = false;
	}
	
	/*
	public static OutputStream getOutputStreamFromFile(File output_file){
		OutputStream output_stream = null;
		try {
			output_stream = new BufferedOutputStream(new FileOutputStream(output_file));
		} catch (FileNotFoundException e) {
			Log.e(TAG, "Camera output file not found");
			e.printStackTrace();
		}
		
		return output_stream;

	}
	*/
	
	public static FileOutputStream getOrCreateFileOutputStream(File output_file){
		if(!output_file.exists()){
			try {
				output_file.createNewFile();
			} catch (IOException e) {
				Log.e(TAG, "New File IOE");
				e.printStackTrace();
			}
		}
		
		try {
			return new FileOutputStream(output_file);
		} catch (FileNotFoundException e) {
			Log.e(TAG, "New FileOutputStream FNFE");
			e.printStackTrace();
		}
		
		return null;
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
