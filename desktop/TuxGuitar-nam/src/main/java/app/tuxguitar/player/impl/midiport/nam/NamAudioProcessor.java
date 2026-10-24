package app.tuxguitar.player.impl.midiport.nam;

import java.io.File;
import java.util.Map;

import app.tuxguitar.midi.synth.TGAudioBuffer;
import app.tuxguitar.midi.synth.TGAudioProcessor;

public class NamAudioProcessor implements TGAudioProcessor {

	// Fallback gdy DLL lub model niedostępny (spike z M1)
	private static final float SPIKE_GAIN = 0.5f;

	private float[][] samples;
	private long nativeHandle;

	public NamAudioProcessor() {
		this.samples = new float[TGAudioBuffer.CHANNELS][TGAudioBuffer.BUFFER_SIZE / 2];
		this.nativeHandle = 0L;
		System.out.println("[TG-NAM] NamAudioProcessor created (native loaded: " + NamNative.LOADED + ")");
	}

	public void open() {
		closeNative();
		String modelPath = findModelPath();
		if (modelPath == null) {
			System.out.println("[TG-NAM] no model file found, using Java fallback");
			return;
		}
		if (!NamNative.LOADED) {
			System.out.println("[TG-NAM] native library missing, cannot load: " + modelPath);
			return;
		}
		this.nativeHandle = NamNative.open(modelPath);
		if (this.nativeHandle != 0L) {
			System.out.println("[TG-NAM] model loaded: " + modelPath);
		} else {
			System.out.println("[TG-NAM] failed to load model: " + modelPath);
		}
	}

	public void close() {
		closeNative();
	}

	public void finalize() {
		this.close();
	}

	public boolean isOpen() {
		return true;
	}

	public boolean isBusy() {
		return false;
	}

	public void fillBuffer(TGAudioBuffer buffer) {
		buffer.read(this.samples);
		float[] left = this.samples[0];
		float[] right = this.samples.length > 1 ? this.samples[1] : this.samples[0];
		int frames = left.length;

		if (this.nativeHandle != 0L) {
			NamNative.process(this.nativeHandle, left, right, frames);
		} else {
			for (int c = 0; c < this.samples.length; c++) {
				float[] channel = this.samples[c];
				for (int i = 0; i < channel.length; i++) {
					channel[i] = channel[i] * SPIKE_GAIN;
				}
			}
		}
		buffer.write(this.samples);
	}

	public void storeParameters(Map<String, String> parameters) {
		// M3: zapis ścieżki modelu .nam, gain, bypass
	}

	public void restoreParameters(Map<String, String> parameters) {
		// M3: odczyt ścieżki modelu .nam, gain, bypass
		// (po M3 restoreParameters wywoła open() z zapisaną ścieżką)
	}

	private void closeNative() {
		if (this.nativeHandle != 0L) {
			NamNative.close(this.nativeHandle);
			this.nativeHandle = 0L;
		}
	}

	private String findModelPath() {
		// 1. system property
		String path = System.getProperty("tuxguitar.nam.model");
		if (path != null && new File(path).isFile()) {
			return path;
		}
		// 2. share/nam/model.nam
		File candidate = new File("share/nam/model.nam");
		if (candidate.isFile()) {
			return candidate.getAbsolutePath();
		}
		// 3. model.nam obok aplikacji
		candidate = new File("model.nam");
		if (candidate.isFile()) {
			return candidate.getAbsolutePath();
		}
		return null;
	}
}