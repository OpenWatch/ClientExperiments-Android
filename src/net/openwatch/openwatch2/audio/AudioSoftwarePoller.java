package net.openwatch.openwatch2.audio;

import java.io.File;
import java.io.IOException;

import net.openwatch.openwatch2.recorder.FFChunkedAudioVideoEncoder;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.AudioRecord.OnRecordPositionUpdateListener;
import android.media.MediaRecorder;
import android.os.AsyncTask;
import android.util.Log;

public class AudioSoftwarePoller {

	public static final String TAG = "AudioRecorder";
		
	public static boolean is_recording = false;
	
	public RecorderTask recorderTask = new RecorderTask();
	
	private FFChunkedAudioVideoEncoder ffencoder;
	
	public AudioSoftwarePoller(FFChunkedAudioVideoEncoder ffencoder){
		this.ffencoder = ffencoder;
	}
	
	public void startRecording() {
		recorderTask.execute();
		//recorder_thread = this.new recorderThread(output_file.getAbsolutePath());
		//recorder_thread.start();
	}

	public void stopRecording() {
		is_recording = false;
		// will stop recording after next sample received
	}
	
	// Thread to run audio sampling in
	public class RecorderTask extends AsyncTask {
		
		//FFAudioEncoder audio_encoder;
			
		private int notification_period; // in samples
				
		int buffer_size = 2 * AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
		
		public int samples_per_frame;
		
		public short[] audio_read_data_buffer;
		public short[] audio_read_data;
		
		public boolean audio_read_data_ready = false; // is audio_read_data safe for reading
		
		public RecorderTask(){
		}
		
		//short[] audio_data = new short[buffer_size];
		

		@Override
		protected Object doInBackground(Object... params) {
			
			//audio_encoder = new FFAudioEncoder();
			
			//int samples_per_frame = audio_encoder.initializeEncoder();
			Log.i(TAG,"audio buffer size: " + String.valueOf(buffer_size) + " samples");
			Log.i(TAG,"audio frame size: " + String.valueOf(samples_per_frame));
			notification_period = samples_per_frame;
			audio_read_data_buffer = new short[samples_per_frame]; // filled directly by hardware
			audio_read_data = new short[samples_per_frame]; // copied after complete frame read
			
			AudioRecord audio_recorder;			
			audio_recorder = new AudioRecord(
					MediaRecorder.AudioSource.MIC, 		 // source
					44100, 								 // sample rate, hz
					AudioFormat.CHANNEL_IN_MONO,		 // channels 
					AudioFormat.ENCODING_PCM_16BIT, 	 // bit depth
					buffer_size); 						 // buffer size
			
			/*
			audio_recorder.setPositionNotificationPeriod(notification_period);
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
					
					//audio_encoder.encodeFrame(audio_data);
					
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
			*/
			is_recording = true;
			audio_recorder.startRecording();
			Log.i("AUDIO_REC","SW recording begin");
			while (is_recording)
	        {
				//Log.i("AUDIO_REC","recording");
				//Log.i("AUDIO_REC", "recording on thread: " + Thread.currentThread().getName());
	            audio_recorder.read(audio_read_data_buffer, 0, samples_per_frame);
	            ffencoder.encodeAudioFrame(audio_read_data_buffer);
	            //audio_read_data_ready = false;
	            //System.arraycopy( audio_read_data_buffer, 0, audio_read_data, 0, audio_read_data_buffer.length );
	            //audio_read_data_ready = true;
	            Log.i("FRAME", "audio frame polled");
	            //audio_encoder.encodeFrame(audio_read_data);
	            // this is sloppy - more data is passed to encodeFrame than is used
	        }
			if(audio_recorder != null){
				audio_recorder.setRecordPositionUpdateListener(null);
				audio_recorder.release();
				audio_recorder = null;
				//audio_encoder.finalizeEncoder();
				Log.i("AUDIO_REC", "stopped");
			}
			return null;
		}
		
	} // recorderThread

} // AudioSoftwareRecorder
