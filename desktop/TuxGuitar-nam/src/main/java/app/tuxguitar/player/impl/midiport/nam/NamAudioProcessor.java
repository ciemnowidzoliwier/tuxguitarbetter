package app.tuxguitar.player.impl.midiport.nam;

import java.io.File;
import java.util.Map;

import app.tuxguitar.midi.synth.TGAudioBuffer;
import app.tuxguitar.midi.synth.TGAudioProcessor;

public class NamAudioProcessor implements TGAudioProcessor {

	public static final String PARAM_MODEL = "nam.model";
	public static final String PARAM_GAIN = "nam.gain";

	private float[][] samples;
	private long nativeHandle;
	private String modelPath;
	private float gain;

	public NamAudioProcessor() {
		this.samples = new float[TGAudioBuffer.CHANNELS][TGAudioBuffer.BUFFER_SIZE / 2];
		this.nativeHandle = 0L;
		this.modelPath = null;
		this.gain = 1.0f;
		System.out.println("[TG-NAM] NamAudioProcessor created (native loaded: " + NamNative.LOADED + ")");
	}

	public void open() {
		// Framework woła restoreParameters() po utworzeniu procesora
		// (TGSynthChannelProcessor.addProcessor) - model ładujemy tam.
	}

	public void close() {
		this.closeNative();
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

	public synchronized void fillBuffer(TGAudioBuffer buffer) {
		buffer.read(this.samples);
		float[] left = this.samples[0];
		float[] right = this.samples.length > 1 ? this.samples[1] : this.samples[0];
		int frames = left.length;

		if (this.nativeHandle != 0L) {
			NamNative.process(this.nativeHandle, left, right, frames);
		}
		if (this.gain != 1.0f) {
			for (int c = 0; c < this.samples.length; c++) {
				float[] channel = this.samples[c];
				for (int i = 0; i < channel.length; i++) {
					channel[i] = channel[i] * this.gain;
				}
			}
		}
		buffer.write(this.samples);
	}

	public void storeParameters(Map<String, String> parameters) {
		parameters.put(PARAM_MODEL, this.modelPath != null ? this.modelPath : "");
		parameters.put(PARAM_GAIN, Float.toString(this.gain));
	}

	public void restoreParameters(Map<String, String> parameters) {
		String model = parameters.get(PARAM_MODEL);
		Float gain = null;
		try {
			gain = Float.valueOf(parameters.get(PARAM_GAIN));
		} catch (Throwable e) {
			gain = null;
		}
		if (gain != null) {
			this.setGain(gain.floatValue());
		}
		this.setModelPath(model);
	}

	public synchronized void setModelPath(String modelPath) {
		String path = null;
		if (modelPath != null && modelPath.trim().length() > 0) {
			path = modelPath.trim();
		}

		// Ten sam model i zaladowany - nic nie robimy
		// (restoreParameters wywolywane przy kazdej aktualizacji programu)
		if (path == null) {
			this.closeNative();
			this.modelPath = null;
			return;
		}
		if (path.equals(this.modelPath) && this.nativeHandle != 0L) {
			return;
		}

		this.closeNative();
		this.modelPath = null;

		File file = new File(path);
		if (!file.isFile()) {
			System.out.println("[TG-NAM] model file not found: " + path);
			return;
		}
		if (!NamNative.LOADED) {
			System.out.println("[TG-NAM] native library missing, cannot load model: " + path);
			return;
		}
		this.nativeHandle = NamNative.open(path);
		if (this.nativeHandle != 0L) {
			this.modelPath = path;
			System.out.println("[TG-NAM] model loaded: " + path);
		} else {
			System.out.println("[TG-NAM] failed to load model: " + path);
		}
	}

	public String getModelPath() {
		return this.modelPath;
	}

	public float getGain() {
		return this.gain;
	}

	public void setGain(float gain) {
		this.gain = Math.max(0.0f, Math.min(2.0f, gain));
	}

	private synchronized void closeNative() {
		if (this.nativeHandle != 0L) {
			NamNative.close(this.nativeHandle);
			this.nativeHandle = 0L;
		}
	}
}