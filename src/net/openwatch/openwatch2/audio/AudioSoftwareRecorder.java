package net.openwatch.openwatch2.audio;

import java.io.File;
import java.io.IOException;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

public class AudioSoftwareRecorder {

	public static final String TAG = "AudioRecorder";

	private static AudioRecord audio_recorder;

	public static void startRecording(File output_file) {
		audio_recorder = new AudioRecord(
				MediaRecorder.AudioSource.MIC, // source
				44100, 								 // sample rate, hz
				AudioFormat.CHANNEL_IN_MONO,		 // channels 
				AudioFormat.ENCODING_PCM_16BIT, 	 // bit depth
				AudioRecord.getMinBufferSize(44100, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT)); // buffer size
		audio_recorder.startRecording();
		
		
	}

	public static void stopRecording() {
		audio_recorder.stop();
		audio_recorder.release();
		audio_recorder = null;

	}

}
