package net.openwatch.openwatch2.video;

public class FFVideoEncoder {
	
	static {
    	System.loadLibrary("FFVideoEncoder");
    }
	
	public static native void testFFMPEG(String filename);
	
	public native void initializeEncoder(String filename, int width, int height);
	public native void encodeFrame(byte[] data);
	public native void finalizeEncoder();

}
