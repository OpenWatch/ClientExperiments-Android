package net.openwatch.openwatch2.recorder;

/* 
 * Chunk output files for piecewise upload.
 * Flow:
 * 1. initializeEncoder(..)
 * 2. encodeFrame()
 * 3. shiftEncoders(.) to begin recording to new chunk.
 * 4. finalizeEncoder()
 */
public class FFChunkedAudioVideoEncoder {
	
	static {
    	System.loadLibrary("FFChunkedAudioVideoEncoder");
    }
	
	public String output_filename;
	
	/*
	 * Returns the audio frame size in samples
	 */
	public int initializeEncoder(String filename1, String filename2, int width, int height, int fps){
		output_filename = filename1;
		return internalInitializeEncoder(filename1, filename2, width, height, fps);
	}
	
	public static native void testFFMPEG(String filename);
	
	public native int internalInitializeEncoder(String filename1, String filename2, int width, int height, int fps);
	public native void shiftEncoders(String new_filename);
	public native void processAVData(byte[] video_frame, long timestamp, short[] audio_data, int audio_length);
	//public native void encodeVideoFrame(byte[] video_data, long timestamp);
	//public native void encodeAudioFrame(short[] audio_data);
	public native void finalizeEncoder(int is_final); // 0: false, !0: true

}
