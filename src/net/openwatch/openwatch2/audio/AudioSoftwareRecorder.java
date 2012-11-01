package net.openwatch.openwatch2.audio;

import java.io.File;
import java.io.IOException;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.AudioRecord.OnRecordPositionUpdateListener;
import android.media.MediaRecorder;
import android.os.AsyncTask;
import android.util.Log;

public class AudioSoftwareRecorder {

	public static final String TAG = "AudioRecorder";
		
	public static boolean is_recording = false;
	
	public void startRecording(File output_file) {
		RecorderTask audio_recorder_task = new RecorderTask(output_file.getAbsolutePath());
		audio_recorder_task.execute();
		//recorder_thread = this.new recorderThread(output_file.getAbsolutePath());
		//recorder_thread.start();

	}

	public void stopRecording() {
		is_recording = false;
		// will stop recording after next sample received
	}
	
	// Thread to run audio sampling in
	public class RecorderTask extends AsyncTask {
	
		private String filename;
		
		private final int NOTIFICATION_PERIOD = 80; // in samples
				
		int buffer_size = 2 * AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
		
		public RecorderTask(String filename){
			this.filename = filename;	
		}
		
		short[] audio_data = new short[buffer_size];

		@Override
		protected Object doInBackground(Object... params) {
			
			//FFAudioEncoder
			
			AudioRecord audio_recorder;
			Log.i("AUDIO_REC","init. buffer size: " + String.valueOf(buffer_size));
			
			audio_recorder = new AudioRecord(
					MediaRecorder.AudioSource.MIC, 		 // source
					44100, 								 // sample rate, hz
					AudioFormat.CHANNEL_IN_MONO,		 // channels 
					AudioFormat.ENCODING_PCM_16BIT, 	 // bit depth
					buffer_size); 						 // buffer size
			
			
			audio_recorder.setPositionNotificationPeriod(NOTIFICATION_PERIOD);
			audio_recorder.setRecordPositionUpdateListener(new OnRecordPositionUpdateListener(){

				@Override
				public void onMarkerReached(AudioRecord audio_recorder) {
					// unused
					Log.i("AUDIO_REC", "onMarkerReached");
				}

				@Override
				public void onPeriodicNotification(AudioRecord audio_recorder) {
					//audio_recorder.read(audio_data, 0, buffer_size); // blocks UI thread
					// send audio_data to ffmpeg
					
					
					
					Log.i("AUDIO_REC", "onPeriodicNotification: " + String.valueOf(audio_data));
					//Log.i("AUDIO_REC", "periodicNotification on thread: " + Thread.currentThread().getName());
					//if(!is_recording){
					if(!is_recording && audio_recorder != null){
						audio_recorder.setRecordPositionUpdateListener(null);
						audio_recorder.release();
						audio_recorder = null;
						Log.i("AUDIO_REC", "Audio polling stopped");
					}
				}
				
			});
			
			is_recording = true;
			audio_recorder.startRecording();
			Log.i("AUDIO_REC","SW recording begin");
			while (is_recording)
	        {
				//Log.i("AUDIO_REC","recording");
				Log.i("AUDIO_REC", "recording on thread: " + Thread.currentThread().getName());
	            audio_recorder.read(audio_data, 0, buffer_size);
	        }
			if(audio_recorder != null){
				audio_recorder.setRecordPositionUpdateListener(null);
				audio_recorder.release();
				audio_recorder = null;
			}
			return null;
		}
		
	} // recorderThread

} // AudioSoftwareRecorder
