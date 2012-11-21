package net.openwatch.openwatch2;

import java.io.File;
import java.util.Date;

import net.openwatch.openwatch2.camera.CameraPreview;
import net.openwatch.openwatch2.recorder.ChunkedAudioVideoSoftwareRecorder;
import net.openwatch.openwatch2.video.FFVideoEncoder;
import net.openwatch.openwatch2.video.VideoSoftwareRecorder;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.FrameLayout;

public class MainActivity extends Activity {

	private static String output_filename = "";

	private Camera mCamera;
	private CameraPreview mPreview;
	
	private static final String OUTPUT_DIR = "/sdcard/ffmpeg_testing/";

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

		// Create an instance of Camera
		mCamera = getCameraInstance();

		// Create our Preview view and set it as the content of our activity.
		//mPreview = new CameraPreview(this, mCamera);

		Button test_btn = (Button) findViewById(R.id.test_btn);
		test_btn.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				/*
				File output_dir_file = new File(OUTPUT_DIR);
				if (!output_dir_file.exists())
					output_dir_file.mkdir();
				output_filename = OUTPUT_DIR + String.valueOf(new Date().getTime());
				FFVideoEncoder.testFFMPEG(output_filename + "H264.mp4");
					//VideoSoftwareRecorder.startRecording((SurfaceView) MainActivity.this.findViewById(R.id.camera_surface_view), new File(output_filename));
				*/
				
				
				if (av_recorder.is_recording) {
					av_recorder.stopRecording();
					((Button) v).setText("Start Recording");
				} else {
					// Start camera preview
					//FrameLayout preview = (FrameLayout) findViewById(R.id.camera_preview);
					//preview.addView(mPreview);
					
					File output_dir_file = new File(OUTPUT_DIR);
					if (!output_dir_file.exists())
						output_dir_file.mkdir();
					output_filename = OUTPUT_DIR + String.valueOf(new Date().getTime());
					try {
						av_recorder.startRecording(mCamera,
												  (SurfaceView) MainActivity.this.findViewById(R.id.camera_surface_view),
												  output_filename);
					} catch (Exception e) {
						Log.e("Recorder init error", "Could not init av_recorder");
						e.printStackTrace();
					}

					((Button) v).setText("Stop Recording");
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
	}

	/** A safe way to get an instance of the Camera object. */
	public static Camera getCameraInstance() {
		Camera c = null;
		try {
			c = Camera.open(); // attempt to get a Camera instance
		} catch (Exception e) {
			// Camera is not available (in use or does not exist)
		}
		return c; // returns null if camera is unavailable
	}
}
