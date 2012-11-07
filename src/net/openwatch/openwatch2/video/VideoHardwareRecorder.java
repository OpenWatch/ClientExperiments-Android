package net.openwatch.openwatch2.video;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import net.openwatch.openwatch2.MainActivity;
import net.openwatch.openwatch2.R;

import android.annotation.SuppressLint;
import android.hardware.Camera;
import android.media.MediaRecorder;
import android.media.MediaRecorder.OnInfoListener;
import android.util.Log;
import android.view.SurfaceView;

public class VideoHardwareRecorder {
	private static final String TAG = "VideoHardwareRecorder";
	
	public static MediaRecorder video_recorder;
	public static Camera camera; 
	
	public static boolean is_recording = false;
	
	private static FileOutputStream camera_output_stream;
	
	// serves as file chunk suffix
	private static int chunk_count = 0;
	
	/** 
	 * Begin recording video the the output_file specified.
	 * @param camera_surface_view the SurfaceView to attach the camera preview to
	 * @param output_file the destination of the recording. This file will be created if it doesn't
	 * all ready exist.
	 */
	@SuppressLint("NewApi")
	public static void startRecording(SurfaceView camera_surface_view, String output_path){
		final SurfaceView final_camera_surface_view = camera_surface_view;
		final String final_output_path = output_path; 
		//camera_output_stream = getOutputStreamFromFile(output_file);
		chunk_count++;
		// This is not buffered
		camera_output_stream = getOrCreateFileOutputStream(output_path + "_" + String.valueOf(chunk_count));
		
		Log.i(TAG,"startRecording " + output_path + "_" + String.valueOf(chunk_count));

		if(camera == null)
			camera = Camera.open();
		else
			return; // The last video recording was not stopped properly
		
		// TODO: Camera setup method: autofocus, setRecordingHint etc.
		Camera.Parameters params = camera.getParameters();
		params.setRecordingHint(true);
		camera.setParameters(params);
		
		try {
			camera.setPreviewDisplay(camera_surface_view.getHolder());
		} catch (IOException e) {
			Log.e(TAG, "setPreviewDisplay IOE");
			e.printStackTrace();
		}
		
		camera.startPreview();
		camera.unlock();
		
		video_recorder = new MediaRecorder();
		/*
		video_recorder.setOnInfoListener(new OnInfoListener(){

			@Override
			public void onInfo(MediaRecorder mr, int what, int extra) {
				if(what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_DURATION_REACHED){
					//mr.stop();
					Log.i(TAG, "max duration reached. Is recording: " + String.valueOf(VideoHardwareRecorder.is_recording));
					chunk_count ++;
					//String next_chunk_output_path = final_output_path + String.valueOf(chunk_count);
					VideoHardwareRecorder.startRecording(final_camera_surface_view, final_output_path);
				}
				
			}
			
		});
		*/
		
		video_recorder.setCamera(camera);
		video_recorder.setAudioSource(MediaRecorder.AudioSource.CAMCORDER);
		video_recorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
		
		video_recorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
		video_recorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);
		video_recorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
		
		//video_recorder.setMaxDuration(1000*5);
		
		try {
			video_recorder.setOutputFile(camera_output_stream.getFD());
		} catch (IllegalStateException e) {
			Log.e(TAG, "setOutputFile ISE");
			e.printStackTrace();
		} catch (IOException e) {
			Log.e(TAG, "setOutputFile IOE");
			e.printStackTrace();
		}
		
		video_recorder.setPreviewDisplay(camera_surface_view.getHolder().getSurface());
		
		try {
			video_recorder.prepare();
		} catch (IllegalStateException e) {
			Log.e(TAG, "video_recorder.prepare ISE");
			e.printStackTrace();
		} catch (IOException e) {
			Log.e(TAG, "video_recorder.prepare IOE");
			e.printStackTrace();
		}
		
		video_recorder.start();
		is_recording = true;

	}
	
	public static void stopRecording(){
		video_recorder.stop();
		video_recorder.release();
		camera.lock();
		camera.stopPreview();
		camera.release();
		camera = null;
		//chunk_count = 0;
		try {
			camera_output_stream.close();
		} catch (IOException e) {
			Log.e(TAG, "close camera_output_stream IOE");
			e.printStackTrace();
		}
		
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
	
	public static FileOutputStream getOrCreateFileOutputStream(String output_path){
		File output_file = new File(output_path);
		
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
	
}
