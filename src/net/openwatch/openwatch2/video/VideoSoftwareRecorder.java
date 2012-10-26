package net.openwatch.openwatch2.video;

import java.io.File;
import java.io.IOException;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.SurfaceView;

public class VideoSoftwareRecorder {

	private static final String TAG = "VideoSoftwareRecorder";
	public static Camera camera;
	
	public static boolean is_recording = false;

	public static void startRecording(SurfaceView camera_surface_view,
			File output_file) {

		if (camera == null)
			camera = Camera.open();
		else
			return; // The last video recording was not stopped properly

		
		try {
			camera.setPreviewDisplay(camera_surface_view.getHolder());
		} catch (IOException e) {
			Log.e(TAG, "setPreviewDisplay IOE");
			e.printStackTrace();
		}
		
		Size previewSize = camera.getParameters().getPreviewSize();
		int dataBufferSize=(int)(previewSize.height*previewSize.width*
                (ImageFormat.getBitsPerPixel(camera.getParameters().getPreviewFormat())/8.0));
		
		camera.setPreviewCallback(new Camera.PreviewCallback() {
			
			@Override
			public void onPreviewFrame(byte[] data, Camera camera) {
				Log.d(TAG,"Frame received");
			}
		});
		
		camera.startPreview();
		is_recording = true;
	}

	public static void stopRecording() {
		camera.stopPreview();
		camera.setPreviewCallback(null);
		camera.release();
		camera = null;
		is_recording = false;
	}

}
