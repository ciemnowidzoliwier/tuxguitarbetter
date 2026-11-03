package app.tuxguitar.player.impl.midiport.vst;

import app.tuxguitar.midi.synth.TGAudioBuffer;
import app.tuxguitar.midi.synth.TGAudioProcessor;
import app.tuxguitar.midi.synth.TGSynthChannel;
import app.tuxguitar.midi.synth.impl.vst.jni.VSTInstance;
import app.tuxguitar.midi.synth.impl.vst.jni.VSTPlugin;
import app.tuxguitar.midi.synth.impl.vst.jni.VSTWorld;
import app.tuxguitar.util.TGContext;

public class VSTAudioProcessor implements TGAudioProcessor {

	private TGContext context;
	private VSTWorld vstWorld;
	private String programId;
	private String pluginFormat;
	private VSTInstance instance;
	private double sampleRate = 44100.0;
	private int blockSize = 512;

	public VSTAudioProcessor(TGContext context, VSTWorld vstWorld, String programId, String pluginFormat) {
		this.context = context;
		this.vstWorld = vstWorld;
		this.programId = programId;
		this.pluginFormat = pluginFormat;
		initializePlugin();
	}

	private void initializePlugin() {
		if (this.vstWorld == null) {
			return;
		}

		// Parse program ID: path|componentId
		String[] parts = this.programId.split("\\|", 2);
		if (parts.length != 2) {
			return;
		}

		String pluginPath = parts[0];
		String componentId = parts[1];

		// Find the plugin and component
		for (VSTPlugin plugin : this.vstWorld.getPlugins()) {
			if (plugin.getPath().equals(pluginPath) && plugin.getType().equals(this.pluginFormat)) {
				for (app.tuxguitar.midi.synth.impl.vst.jni.VSTComponent component : plugin.getComponents()) {
					if (component.getId().equals(componentId)) {
						long instancePtr = createInstance(plugin.getPath(), componentId);
                        if (instancePtr != 0) {
                            this.instance = new VSTInstance(instancePtr);
                        }
						return;
					}
				}
			}
		}
	}

	private native long createInstance(String pluginPath, String componentId);

	@Override
	public void initialize(double sampleRate, int blockSize) {
		this.sampleRate = sampleRate;
		this.blockSize = blockSize;
		if (this.instance != null) {
			this.instance.initialize(sampleRate, blockSize);
		}
	}

	@Override
	public void terminate() {
		if (this.instance != null) {
			this.instance.terminate();
		}
	}

	@Override
	public void processAudio(TGAudioBuffer buffer) {
		if (this.instance != null) {
			int numChannels = buffer.getChannelCount();
			int numSamples = buffer.getSampleCount();

			// Convert TGAudioBuffer to float[][]
			float[][] inputs = new float[numChannels][numSamples];
			float[][] outputs = new float[numChannels][numSamples];

			for (int ch = 0; ch < numChannels; ch++) {
				for (int i = 0; i < numSamples; i++) {
					inputs[ch][i] = buffer.getSample(ch, i);
				}
			}

			// Process audio through VST
			this.instance.processAudio(inputs, outputs, numSamples);

			// Copy output back to buffer
			for (int ch = 0; ch < numChannels; ch++) {
				for (int i = 0; i < numSamples; i++) {
					buffer.setSample(ch, i, outputs[ch][i]);
				}
			}
		}
	}

	@Override
	public void processMidi(byte[] data, int offset, int length, int sampleOffset) {
		if (this.instance != null) {
			this.instance.processMidi(data, offset, length, sampleOffset);
		}
	}

	@Override
	public void setParameter(int index, float value) {
		if (this.instance != null) {
			this.instance.setParameter(index, value);
		}
	}

	@Override
	public float getParameter(int index) {
		if (this.instance != null) {
			return this.instance.getParameter(index);
		}
		return 0.0f;
	}

	@Override
	public int getParameterCount() {
		if (this.instance != null) {
			return this.instance.getParameterCount();
		}
		return 0;
	}

	@Override
	public String getParameterName(int index) {
		if (this.instance != null) {
			return this.instance.getParameterName(index);
		}
		return "";
	}

	@Override
	public String getParameterLabel(int index) {
		if (this.instance != null) {
			return this.instance.getParameterLabel(index);
		}
		return "";
	}

	@Override
	public void setProgram(int program) {
		if (this.instance != null) {
			this.instance.setProgram(program);
		}
	}

	@Override
	public int getProgram() {
		if (this.instance != null) {
			return this.instance.getProgram();
		}
		return 0;
	}

	@Override
	public int getProgramCount() {
		if (this.instance != null) {
			return this.instance.getProgramCount();
		}
		return 0;
	}

	@Override
	public String getProgramName(int program) {
		if (this.instance != null) {
			return this.instance.getProgramName(program);
		}
		return "";
	}
}