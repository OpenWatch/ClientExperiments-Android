package net.openwatch.openwatch2;

import java.io.File;
import java.io.IOException;
import java.util.Date;

import net.openwatch.openwatch2.audio.AudioHardwareRecorder;
import net.openwatch.openwatch2.audio.AudioSoftwareRecorder;
import net.openwatch.openwatch2.audio.AudioStreamer;
import net.openwatch.openwatch2.audio.FFAudioEncoder;
import net.openwatch.openwatch2.constants.OWConstants;
import net.openwatch.openwatch2.file.FileUtils;
import net.openwatch.openwatch2.recorder.ChunkedAudioVideoSoftwareRecorder;
import net.openwatch.openwatch2.video.ChunkedVideoSoftwareRecorder;
import net.openwatch.openwatch2.video.DualVideoRecorder;
import net.openwatch.openwatch2.video.VideoHardwareRecorder;
import net.openwatch.openwatch2.video.VideoSoftwareRecorder;
import net.openwatch.openwatch2.video.FFVideoEncoder;
import android.os.Build;
import android.os.Bundle;
import android.provider.SyncStateContract.Constants;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.view.Menu;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class MainActivity extends Activity {
	
	private Button record_hw_video_btn;
	private Button record_sw_video_btn;
	
	private static final int max_chunks = 4;
	private static int chunk_counter = 0;
	private static String output_filename = "";
	
	//private ChunkedVideoSoftwareRecorder video_recorder = new ChunkedVideoSoftwareRecorder();
	
	//private AudioSoftwareRecorder audio_software_recorder = new AudioSoftwareRecorder();

	private ChunkedAudioVideoSoftwareRecorder av_recorder = new ChunkedAudioVideoSoftwareRecorder();
	
	@SuppressLint("NewApi")
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		if (Build.VERSION.SDK_INT >= 11) {
			this.getActionBar().setDisplayShowTitleEnabled(false);
			this.getActionBar().setTitle("OW Tech Demo");
		}
		/*
		record_hw_video_btn = (Button) findViewById(R.id.record_hw_video_btn);
		record_hw_video_btn.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				if (VideoHardwareRecorder.is_recording) {
					VideoHardwareRecorder.stopRecording();
					//AudioHardwareRecorder.stopRecording();
					record_sw_video_btn.setEnabled(true);
					((Button) v).setText("Start HW Recording Video");
				} else {
					String video_filename = String.valueOf(new Date().getTime()) + "_AV.mp4";
					File video_output_file = new File(
							FileUtils.getExternalStorage(MainActivity.this,
									OWConstants.recording_directory), video_filename);

					VideoHardwareRecorder.startRecording(
							(SurfaceView) MainActivity.this
									.findViewById(R.id.camera_surface_view),
							video_output_file);
					
					// See if an audio recording can take place simultaneously
					// nope. Throws IllegalStateException
					// Even when a different hardware encoder is used
					// i.e: VideoRecorder using AAC audio hardware and AudioRecorder using AMR hardware
					
					//String audio_filename = String.valueOf(new Date().getTime()) + "_A.3gpp";
					//File audio_output_file = new File(FileUtils.getExternalStorage(MainActivity.this, OWConstants.recording_directory), audio_filename);
					//AudioRecorder.startRecording(audio_output_file);
					
					record_sw_video_btn.setEnabled(false);
					((Button) v).setText("Stop HW Recording Video");
				}

			}

		});
		
		record_sw_video_btn = (Button) findViewById(R.id.record_sw_video_btn);
		record_sw_video_btn.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View v) {
				if(VideoSoftwareRecorder.is_recording){
					VideoSoftwareRecorder.stopRecording();
					record_hw_video_btn.setEnabled(true);
					((Button) v).setText("Start SW Video Recording");
				}
				else{
					String video_filename = String.valueOf(new Date().getTime()) + ".mpg";
					File video_output_file = new File(
							FileUtils.getExternalStorage(MainActivity.this,
									OWConstants.recording_directory), video_filename);

					VideoSoftwareRecorder.startRecording(
							(SurfaceView) MainActivity.this
									.findViewById(R.id.camera_surface_view),
							video_output_file);
					
					record_hw_video_btn.setEnabled(false);
					((Button) v).setText("Stop SW Video Recording");
				}
				
			}
			
		});
		 */
		
		
		
		
		
		
		Button test_btn = (Button) findViewById(R.id.test_btn);
		test_btn.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View v) {
				if(av_recorder.is_recording){
					chunk_counter ++;
					//audio_software_recorder.stopRecording();
					//VideoHardwareRecorder.stopRecording();
					
					//video_recorder.stopRecording();
					//audio_software_recorder.stopRecording();
					
					av_recorder.stopRecording();
					((Button)v).setText("Start Recording");
				
				} else {
					String output_dir = "/sdcard/ffmpeg_testing";
					File output_dir_file = new File(output_dir);
					if(!output_dir_file.exists())
						output_dir_file.mkdir();
					output_filename = output_dir + "/" + String.valueOf(new Date().getTime());
					av_recorder.startRecording((SurfaceView) MainActivity.this
							.findViewById(R.id.camera_surface_view), output_filename);
					//video_recorder.startRecording((SurfaceView) MainActivity.this
					//		.findViewById(R.id.camera_surface_view), output_filename);
					//audio_software_recorder.startRecording(output_filename + ".wav");
					//String output_file_path = output_dir_string + "/" + String.valueOf(new Date().getTime());
					
					//audio_software_recorder.startRecording(output_file_path);
					//VideoHardwareRecorder.startRecording((SurfaceView) MainActivity.this
					//		.findViewById(R.id.camera_surface_view), output_filename);
					//FFAudioEncoder.testFFMPEG(output_filename);
					//audio_software_recorder.startRecording(new File(output_dir));
					((Button)v).setText("Stop Recording");
				}
			}
			
		});
		
		
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// getMenuInflater().inflate(R.menu.activity_main, menu);
		return true;
	}

	@Override
	public void onPause() {
		super.onPause();

		/*
		 * // Release the MediaRecorder if (AudioStreamer.recorder != null) {
		 * AudioStreamer.recorder.release(); AudioStreamer.recorder = null; }
		 */
	}
}
