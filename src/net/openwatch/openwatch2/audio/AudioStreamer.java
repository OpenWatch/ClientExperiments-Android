package net.openwatch.openwatch2.audio;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import android.media.MediaRecorder;
import android.os.ParcelFileDescriptor;

/* MediaRecorder gets mic audio, binds to socket
 * managed by startStreamingAudio() and stopStreamingAudio();
 */
public class AudioStreamer {

	final static String HOSTNAME = "192.168.1.10";
	final static int PORT = 8000;

	public static MediaRecorder recorder;

	// Bind the MediaRecorder to a Socket
	// and commence recording / streaming
	public static void startStreamingAudio() {

		Socket socket = null;
		try {
			socket = new Socket(InetAddress.getByName(HOSTNAME), PORT);
		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}

		ParcelFileDescriptor pfd = ParcelFileDescriptor.fromSocket(socket);

		if (recorder == null) {
			recorder = new MediaRecorder();
		}

		recorder.setAudioSource(MediaRecorder.AudioSource.MIC);
		recorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
		recorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
		// recorder.setAudioEncodingBitRate(128*1000);
		recorder.setOutputFile(pfd.getFileDescriptor());

		try {
			recorder.prepare();
		} catch (IllegalStateException e) {

			e.printStackTrace();
		} catch (IOException e) {

			e.printStackTrace();
		}
		recorder.start();
	}

	public static void stopStreamingAudio() {
		if (recorder != null) {
			recorder.stop();
			recorder.reset(); // You can reuse the object by going back to
			recorder = null;				// setAudioSource() step
		}
	}

}
