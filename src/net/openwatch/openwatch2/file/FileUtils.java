package net.openwatch.openwatch2.file;

import java.io.File;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;
import android.util.Log;

public class FileUtils {
	
	private static final String TAG = "FileUtils";
	
	/**
	 * Returns a File representing a directory in the top level 
	 * storage location. Prefers the External Storage partition (SDCard),
	 * falls back to internal storage.
	 * 
	 * @param c the context to use for getting the internal storage directory
	 * @param directory the name of the desired directory
	 * @return
	 */
	public static File getExternalStorage(Context c, String directory){
		
		File result;
		// First, try getting access to the sdcard partition
		if(Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)){
			Log.d(TAG, "Using sdcard");
			result = new File(Environment.getExternalStorageDirectory(), directory);
		} else {
		// Else, use the internal storage directory for this application
			Log.d(TAG, "Using internal storage");
			result = new File(c.getFilesDir(), directory);
		}
		
		if(!result.exists())
			result.mkdir();

		return result;
	}
}
