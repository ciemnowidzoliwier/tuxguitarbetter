package app.tuxguitar.midi.synth.impl.vst.jni;

import java.util.ArrayList;
import java.util.List;

public class VSTWorld extends VSTObject {

	private List<VSTPlugin> plugins;

	public VSTWorld() {
		this.setInstance(malloc());
	}

	public void finalize() {
		if (this.isInitialized()) {
			this.free(this.getInstance());
			this.setInstance(0);
		}
	}

	public List<VSTPlugin> getPlugins() {
		if (this.plugins == null && this.isInitialized()) {
			this.plugins = this.getAllPlugins(this.getInstance());
		}
		return (this.plugins != null ? this.plugins : new ArrayList<VSTPlugin>());
	}

	// Native methods
	private native long malloc();

	private native void free(long instance);

	private native List<VSTPlugin> getAllPlugins(long instance);
}