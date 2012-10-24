package net.openwatch.openwatch2;

import net.openwatch.openwatch2.stream.AudioStreamer;
import android.os.Build;
import android.os.Bundle;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.view.Menu;

public class MainActivity extends Activity {

	@SuppressLint("NewApi")
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		if (Build.VERSION.SDK_INT >= 11) {
			this.getActionBar().setDisplayShowTitleEnabled(false);
			this.getActionBar().setTitle("Audio Streaming Demo");
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// getMenuInflater().inflate(R.menu.activity_main, menu);
		return true;
	}

	@Override
	public void onPause() {
		super.onPause();

		// Release the MediaRecorder
		if (AudioStreamer.recorder != null) {
			AudioStreamer.recorder.release();
			AudioStreamer.recorder = null;
		}
	}
}
