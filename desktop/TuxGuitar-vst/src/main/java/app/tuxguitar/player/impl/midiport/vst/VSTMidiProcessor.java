package app.tuxguitar.player.impl.midiport.vst;

import app.tuxguitar.midi.synth.TGMidiProcessor;
import app.tuxguitar.midi.synth.TGSynthChannel;
import app.tuxguitar.midi.synth.impl.vst.jni.VSTInstance;
import app.tuxguitar.midi.synth.impl.vst.jni.VSTPlugin;
import app.tuxguitar.midi.synth.impl.vst.jni.VSTWorld;
import app.tuxguitar.util.TGContext;

public class VSTMidiProcessor implements TGMidiProcessor {

	private TGContext context;
	private VSTWorld vstWorld;
	private String programId;
	private String pluginFormat;
	private VSTInstance instance;
	private VSTPlugin plugin;
	private int channel = 0;

	public VSTMidiProcessor(TGContext context, VSTWorld vstWorld, String programId, String pluginFormat) {
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
						this.plugin = plugin;
						// Create VST instance - this will be done via JNI
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
	public void open() {
		if (this.instance != null) {
			// Initialize with default sample rate and block size
			this.instance.initialize(44100.0, 512);
		}
	}

	@Override
	public void close() {
		if (this.instance != null) {
			this.instance.terminate();
		}
	}

	@Override
	public void setChannel(int channel) {
		this.channel = channel;
	}

	@Override
	public int getChannel() {
		return this.channel;
	}

	@Override
	public void sendMidiEvent(int status, int data1, int data2) {
		if (this.instance != null) {
			byte[] midiData = new byte[]{(byte) status, (byte) data1, (byte) data2};
			this.instance.processMidi(midiData, 0, midiData.length, 0);
		}
	}

	@Override
	public void sendSysex(byte[] data, int offset, int length) {
		if (this.instance != null) {
			this.instance.processMidi(data, offset, length, 0);
		}
	}

	@Override
	public void allNotesOff() {
		if (this.instance != null) {
			// Send All Notes Off controller (CC 123) on all channels
			for (int ch = 0; ch < 16; ch++) {
				byte[] midiData = new byte[]{(byte) (0xB0 | ch), (byte) 123, (byte) 0};
				this.instance.processMidi(midiData, 0, midiData.length, 0);
			}
		}
	}

	@Override
	public void allSoundOff() {
		if (this.instance != null) {
			// Send All Sound Off controller (CC 120) on all channels
			for (int ch = 0; ch < 16; ch++) {
				byte[] midiData = new byte[]{(byte) (0xB0 | ch), (byte) 120, (byte) 0};
				this.instance.processMidi(midiData, 0, midiData.length, 0);
			}
		}
	}

	@Override
	public void reset() {
		allNotesOff();
		allSoundOff();
	}
}