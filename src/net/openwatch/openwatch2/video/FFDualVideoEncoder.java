package net.openwatch.openwatch2.video;

public class FFDualVideoEncoder {
	
	static {
    	System.loadLibrary("FFDualVideoEncoder");
    }
		
	public native void initializeEncoder(String filepath, int width, int height);
	public native void encodeFrame(byte[] data);
	public native void finalizeEncoder();

}
