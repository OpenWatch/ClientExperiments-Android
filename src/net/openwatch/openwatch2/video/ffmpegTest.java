package net.openwatch.openwatch2.video;

public class ffmpegTest {
	
	static {
    	System.loadLibrary("ffmpegutils");
    }
	
	public static native void testFFMPEG(String filename);

}
