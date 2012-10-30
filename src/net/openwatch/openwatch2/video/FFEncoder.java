package net.openwatch.openwatch2.video;

public class FFEncoder {
	
	static {
    	System.loadLibrary("FFEncoder");
    }
	
	public static native void testFFMPEG(String filename);
	
	public native void initializeEncoder(String filename, int width, int height);
	public native void encodeFrame(byte[] data);
	public native void finalizeEncoder();

}
