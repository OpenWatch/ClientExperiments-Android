package net.openwatch.openwatch2.video;

public class FFVideoEncoder {
	
	static {
    	System.loadLibrary("FFVideoEncoder");
    }
	
	public String output_filename;
	
	public void initializeEncoder(String filename, int width, int height){
		output_filename = filename;
		internalInitializeEncoder(filename, width, height);
	}
	
	public static native void testFFMPEG(String filename);
	
	public native void internalInitializeEncoder(String filename, int width, int height);
	public native void encodeFrame(byte[] data);
	public native void finalizeEncoder();

}
