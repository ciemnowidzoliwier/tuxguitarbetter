package app.tuxguitar.player.impl.midiport.nam;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

final class NamNative {

    public static final boolean LOADED;

    private static native long nOpen(String modelPath);
    private static native void nProcess(long handle, float[] left, float[] right, int frames);
    private static native void nClose(long handle);

    static {
        boolean loaded = false;
        try {
            System.loadLibrary("tuxguitar-nam-jni");
            loaded = true;
            Log.i("TG-NAM", "native library tuxguitar-nam-jni loaded");
        } catch (Throwable e) {
            Log.w("TG-NAM", "native library not available, Java fallback active: " + e);
        }
        LOADED = loaded;
    }

    private NamNative() {
    }

    // Opens a NAM model from assets. Copies to cache dir first since native code needs a file path.
    public static long openFromAssets(Context context, String assetPath) {
        if (!LOADED) {
            return 0L;
        }
        String modelPath = copyAssetToCache(context, assetPath);
        if (modelPath == null) {
            return 0L;
        }
        return nOpen(modelPath);
    }

    // Opens a NAM model from a file path (for user-provided models)
    public static long open(String modelPath) {
        return LOADED ? nOpen(modelPath) : 0L;
    }

    public static void process(long handle, float[] left, float[] right, int frames) {
        if (LOADED && handle != 0L) {
            nProcess(handle, left, right, frames);
        }
    }

    public static void close(long handle) {
        if (LOADED && handle != 0L) {
            nClose(handle);
        }
    }

    // Copy asset to cache directory and return the file path
    private static String copyAssetToCache(Context context, String assetPath) {
        File cacheDir = context.getCacheDir();
        File cacheFile = new File(cacheDir, assetPath.replace('/', '_'));

        // If already cached and not outdated, return cached version
        if (cacheFile.exists()) {
            return cacheFile.getAbsolutePath();
        }

        try (InputStream in = context.getAssets().open(assetPath);
             FileOutputStream out = new FileOutputStream(cacheFile)) {

            byte[] buffer = new byte[8192];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            return cacheFile.getAbsolutePath();
        } catch (IOException e) {
            Log.e("TG-NAM", "Failed to copy asset: " + assetPath, e);
            return null;
        }
    }
}