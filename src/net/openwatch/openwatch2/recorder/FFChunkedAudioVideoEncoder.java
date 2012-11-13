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
	
	public void initializeEncoder(String filename1, String filename2, int width, int height){
		output_filename = filename1;
		internalInitializeEncoder(filename1, filename2, width, height);
	}
	
	public static native void testFFMPEG(String filename);
	
	public native void internalInitializeEncoder(String filename1, String filename2, int width, int height);
	public native void shiftEncoders(String new_filename);
	public native void encodeFrame(byte[] data);
	public native void finalizeEncoder(int is_final); // 0: false, !0: true

}
