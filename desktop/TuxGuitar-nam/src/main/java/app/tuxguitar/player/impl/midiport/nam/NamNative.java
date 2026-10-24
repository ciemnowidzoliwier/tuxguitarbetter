package app.tuxguitar.player.impl.midiport.nam;

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
			System.out.println("[TG-NAM] native library tuxguitar-nam-jni loaded");
		} catch (Throwable e) {
			System.out.println("[TG-NAM] native library not available, Java fallback active: " + e);
		}
		LOADED = loaded;
	}

	private NamNative() {
	}

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
}