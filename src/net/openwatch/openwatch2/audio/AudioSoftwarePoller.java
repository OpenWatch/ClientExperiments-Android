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
	
	public static final int SAMPLE_RATE = 44100;
	public static final int CHANNEL_CONFIG = AudioFormat.CHANNEL_IN_MONO;
	public static final int AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
	
	public static boolean is_recording = false;
	
	public RecorderTask recorderTask = new RecorderTask();
		
	// reused readAudioFrames() variables
	int read_index;
	int write_index;
	int distance; // difference between write / read indexes
	public int read_distance; // the greatest multiple of samples_per_frame < distance
	int tail_distance; // if the buffer_write_index < buffer_read_index, how many items shall
					   // we copy from the buffer tail before resuming from the buffer head
	
	public AudioSoftwarePoller(){
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
	
	/*
	 * Reads from the circular buffer.
	 * The short[] returned has length equal to 
	 * a multiple of samples_per_frame.
	 */
	public short[] readAudioFrames(){
		short[] audio_samples = null;
		
		read_index = recorderTask.buffer_read_index;
		write_index = recorderTask.buffer_write_index;
		
		if(write_index == 0){
			Log.i("AUDIO_READ_BUFFER", "Buffer empty");
			read_distance = 0;
			return audio_samples; // if samples aren't ready, there's nothing to do
		}
		// Compute distance between read & write indexes in circular buffer
		if(write_index < read_index)
			distance = recorderTask.buffer_size - Math.abs((write_index - read_index));
		else
			distance = write_index - read_index;
		
		read_distance = (distance / recorderTask.samples_per_frame) * recorderTask.samples_per_frame;
		
		audio_samples = new short[read_distance];
		
		if(write_index < read_index){
			tail_distance = recorderTask.buffer_size - read_index;
			// copy from buffer_read_index to end of buffer
			System.arraycopy(recorderTask.data_buffer, read_index, audio_samples, 0, tail_distance);
			// copy from start of buffer to buffer_write_index
			System.arraycopy(recorderTask.data_buffer, 0, audio_samples, tail_distance-1, read_distance - tail_distance);
		}else
			System.arraycopy(recorderTask.data_buffer, read_index, audio_samples, 0, read_distance);
		
		Log.i("AUDIO_READ_BUFFER",String.valueOf(read_index) + " - " + String.valueOf(write_index-1) + " dist: "+ String.valueOf(read_distance));
		
		recorderTask.buffer_read_index = write_index;
		recorderTask.total_frames_read += (distance / recorderTask.samples_per_frame);
		return audio_samples;
	}
	
	
	
	// Thread to run audio sampling in
	public class RecorderTask extends AsyncTask {
		
		//FFAudioEncoder audio_encoder;
			
		private int notification_period; // in samples
				
		//int buffer_size = 2 * AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
		public int buffer_size;
		
		public int samples_per_frame; // set before RecorderTask.execute();
		public final int FRAMES_PER_BUFFER = 43; // 1 sec @ 1024 samples/frame (aac)
		
		public int buffer_write_index = 0; // buffer write-in point
		public int buffer_read_index = 0; // buffer read-in point
		
		public short[] data_buffer;
		
		public int total_frames_written = 0;
		public int total_frames_read = 0;
		
		//public boolean audio_read_data_ready = false; // is audio_read_data safe for reading
		
		public boolean first_pass_complete = false;
		
		public RecorderTask(){
		}
		
		//short[] audio_data = new short[buffer_size];
		

		@Override
		protected Object doInBackground(Object... params) {
			
			int min_buffer_size = AudioRecord.getMinBufferSize(SAMPLE_RATE, CHANNEL_CONFIG, AUDIO_FORMAT);

			buffer_size = samples_per_frame * FRAMES_PER_BUFFER;
			
			// Some audio codecs have extremeley small frame sizes
			// ensure our buffer is adequately sized
			if(buffer_size < min_buffer_size)
				buffer_size = ((min_buffer_size / samples_per_frame) + 1) * samples_per_frame * 2;
			
			//Log.i(TAG,"audio buffer size: " + String.valueOf(buffer_size) + " samples, min: " + String.valueOf(min_buffer_size));
			//Log.i(TAG,"audio frame size: " + String.valueOf(samples_per_frame));
			notification_period = samples_per_frame;
			data_buffer = new short[buffer_size]; // filled directly by hardware
			
			AudioRecord audio_recorder;			
			audio_recorder = new AudioRecord(
					MediaRecorder.AudioSource.MIC, 		 // source
					SAMPLE_RATE, 						 // sample rate, hz
					CHANNEL_CONFIG,		 				 // channels 
					AUDIO_FORMAT, 	 					 // audio format
					buffer_size);	 					 // buffer size (bytes)
			
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
					first_pass_complete = true;
					//audio_encoder.encodeFrame(audio_data);
					
					//Log.i("AUDIO_REC", "onPeriodicNotification: " + String.valueOf(audio_data));
					//Log.i("AUDIO_REC", "periodicNotification on thread: " + Thread.currentThread().getName());
					//if(!is_recording){
					
					//if(!is_recording && audio_recorder != null){
					//	audio_recorder.setRecordPositionUpdateListener(null);
					//	audio_recorder.release();
					//	audio_recorder = null;
					//	Log.i("AUDIO_REC", "Audio polling stopped");
					//}
				}
				
			});*/
			
			is_recording = true;
			audio_recorder.startRecording();
			Log.i("AUDIO_REC","SW recording begin");
			while (is_recording)
	        {
				//Log.i("AUDIO_REC","recording");
				//Log.i("AUDIO_REC", "recording on thread: " + Thread.currentThread().getName());
	            audio_recorder.read(data_buffer, buffer_write_index, samples_per_frame);
	            //Log.i("AUDIO_FILL_BUFFER",String.valueOf(buffer_write_index) + " - " + String.valueOf(buffer_write_index + samples_per_frame-1));
	            buffer_write_index = (buffer_write_index + samples_per_frame) % buffer_size;
	            total_frames_written ++;
	            //ffencoder.encodeAudioFrame(audio_read_data_buffer);
	            //audio_read_data_ready = false;
	            //System.arraycopy( audio_read_data_buffer, 0, audio_read_data, 0, audio_read_data_buffer.length );
	            //audio_read_data_ready = true;
	            
	            //Log.i("FRAME", "audio frame polled");
	            
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
