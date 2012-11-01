package net.openwatch.openwatch2.audio;

public class FFAudioEncoder {
	
	static {
    	System.loadLibrary("FFAudioEncoder");
    }
	
	public static native void testFFMPEG(String filename);
	public native int initializeEncoder(String filename);
	public native void encodeFrame(byte[] frame_data);
	public native void initializeEncoder();

}
