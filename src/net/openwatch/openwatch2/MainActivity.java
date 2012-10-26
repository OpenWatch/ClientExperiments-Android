package net.openwatch.openwatch2;

import java.io.File;
import java.util.Date;

import net.openwatch.openwatch2.audio.AudioRecorder;
import net.openwatch.openwatch2.audio.AudioStreamer;
import net.openwatch.openwatch2.constants.OWConstants;
import net.openwatch.openwatch2.file.FileUtils;
import net.openwatch.openwatch2.video.VideoRecorder;
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

	@SuppressLint("NewApi")
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		if (Build.VERSION.SDK_INT >= 11) {
			this.getActionBar().setDisplayShowTitleEnabled(false);
			this.getActionBar().setTitle("OW Tech Demo");
		}

		Button record_video_btn = (Button) findViewById(R.id.record_video_btn);
		record_video_btn.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				if (VideoRecorder.is_recording) {
					VideoRecorder.stopRecording();
					//AudioRecorder.stopRecording();
					((Button) v).setText("Start Recording Video");
				} else {
					String video_filename = String.valueOf(new Date().getTime()) + "_AV.mp4";
					File video_output_file = new File(
							FileUtils.getExternalStorage(MainActivity.this,
									OWConstants.recording_directory), video_filename);

					VideoRecorder.startRecording(
							(SurfaceView) MainActivity.this
									.findViewById(R.id.camera_surface_view),
							video_output_file);
					
					// See if an audio recording can take place simultaneously
					// nope. Throws IllegalStateException
					/*
					String audio_filename = String.valueOf(new Date().getTime()) + "_A.mp4";
					File audio_output_file = new File(FileUtils.getExternalStorage(MainActivity.this, OWConstants.recording_directory), audio_filename);
					AudioRecorder.startRecording(audio_output_file);
					*/
					((Button) v).setText("Stop Recording Video");
				}

			}

		});
		
		Button read_output_stream_btn = (Button) findViewById(R.id.read_output_stream_btn);
		read_output_stream_btn.setEnabled(false);
		read_output_stream_btn.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View v) {
				if(VideoRecorder.is_recording){
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
