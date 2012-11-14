package net.openwatch.openwatch2.recorder;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.List;

import net.openwatch.openwatch2.audio.AudioSoftwarePoller;
import net.openwatch.openwatch2.audio.AudioSoftwareRecorder;
import net.openwatch.openwatch2.video.FFChunkedVideoEncoder;

import android.annotation.SuppressLint;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.SurfaceView;

public class ChunkedAudioVideoSoftwareRecorder {

	private static final String TAG = "ChunkedVideoSoftwareRecorder";
	public static Camera camera;
	
	public static boolean is_recording = false;
	
	/*
	private static FFVideoEncoder ffencoder1;
	private static FFVideoEncoder ffencoder2;
	private static FFVideoEncoder current_encoder;
	*/
	private static FFChunkedAudioVideoEncoder ffencoder;
	
	private int chunk = 1; // count video chunks in this recording
	
	private int chunk_frame_count = 0;
	private int chunk_frame_max = 25*5; // chunk every this many frames
	
	private String output_filename_base = "";
	
	private final String file_ext = ".mp4";
	private final int output_width = 320;
	private final int output_height = 240;
	
	private AudioSoftwarePoller audio_recorder;

	
	@SuppressLint("NewApi")
	public void startRecording(SurfaceView camera_surface_view,
			String output_filename_base) {
		
		chunk = 1;
		
		this.output_filename_base = output_filename_base;
		/*
		ffencoder1 = new FFVideoEncoder();
		ffencoder1.initializeEncoder(output_filename_base + "_" + String.valueOf(chunk + 1) + file_ext, output_width, output_height);
		
		ffencoder2 = new FFVideoEncoder();
		ffencoder2.initializeEncoder(output_filename_base + "_" + String.valueOf(chunk) + file_ext, output_width, output_height);
		
		current_encoder = ffencoder2;
		 */
		
		ffencoder = new FFChunkedAudioVideoEncoder();
		int num_samples = ffencoder.initializeEncoder(output_filename_base + "_" + String.valueOf(chunk) + file_ext, 
				output_filename_base + "_" + String.valueOf(chunk + 1) + file_ext,
				output_width, output_height);
		
		audio_recorder = new AudioSoftwarePoller(ffencoder);
		audio_recorder.recorderTask.samples_per_frame = num_samples;
		Log.i("AUDIO_FRAME_SIZE", "audio frame size: "+ String.valueOf(num_samples));
		audio_recorder.startRecording();
		
		// ensure A frame is ready before commencing video preview
		// hacky. Get rid of it
		while(audio_recorder.recorderTask.audio_read_data == null){
			continue;
		}
		
		chunk += 2;
		
		if (camera == null)
			camera = Camera.open();
		else
			return; // The last video recording was not stopped properly
		
		Camera.Parameters camera_parameters = camera.getParameters();
		camera_parameters.setPreviewFormat(ImageFormat.NV21);
		camera_parameters.setPreviewSize(output_width, output_height);
		List<int[]> fpsRanges = camera_parameters.getSupportedPreviewFpsRange();
		camera_parameters.setPreviewFpsRange(15000,15000);
		//int max_fps = camera_parameters.getSupportedPreviewFpsRange().get(Camera.Parameters.PREVIEW_FPS_MAX_INDEX);
		//int min_fps = 
		//camera_parameters.setPreviewFpsRange(min, max)
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
			public void onPreviewFrame(byte[] video_frame_data, Camera camera) {
				Log.d("FRAME","video frame polled");
				//while(!audio_recorder.recorderTask.audio_read_data_ready)
					//continue; // make sure we aren't writing to audio_read_data
				ffencoder.encodeVideoFrame(video_frame_data);
				// audio_recorder.recorderTask.audio_read_data
				chunk_frame_count++;
				if(chunk_frame_count >= chunk_frame_max){
					chunk_frame_count = 0;
					swapEncoders();
				}
			}
		});
		
		camera.startPreview();
		is_recording = true;
	}
	
	private void swapEncoders(){
		int current_encoder_int = chunk % 2 + 1; // returns 1 or 2
		// chunk=3, current_encoder_int = 2
		//Log.i(TAG,"chunk " + String.valueOf(chunk-2) + " complete at: " + current_encoder.output_filename);
		
		//ffencoder.finalizeEncoder(0);
		ffencoder.shiftEncoders(getFilePath(output_filename_base + "_" + String.valueOf(chunk) + file_ext));
		
		/*
		if(current_encoder == ffencoder1)
			Log.i(TAG,"encoder 1 completed chunk " + String.valueOf(chunk-2) + " at: " + current_encoder.output_filename);
		else if (current_encoder == ffencoder2)
			Log.i(TAG,"encoder 2 completed chunk " + String.valueOf(chunk-2) + " at: " + current_encoder.output_filename);
		
		// do in another thread
		current_encoder.finalizeEncoder();
		current_encoder.initializeEncoder(getFilePath(output_filename_base + "_" + String.valueOf(chunk) + file_ext), output_width, output_height);
		
		if(current_encoder_int == 1){
			current_encoder = ffencoder2;
		}
		else{
			current_encoder = ffencoder1;
		}
		*/
		//Log.i(TAG,"writing next chunk to: " + current_encoder.output_filename);
		
		chunk ++;
	}

	public void stopRecording() {
		camera.stopPreview();
		camera.setPreviewCallback(null);
		//current_encoder.finalizeEncoder();
		audio_recorder.stopRecording();
		ffencoder.finalizeEncoder(1);
		camera.release();
		camera = null;
		is_recording = false;
	}
	
	public static String getFilePath(String output_filename){
		File output_file = new File(output_filename);
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
