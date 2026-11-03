package app.tuxguitar.midi.synth.impl.vst.jni;

import java.util.ArrayList;
import java.util.List;

public class VSTPlugin extends VSTObject {

	private List<VSTComponent> components;
	private String path;
	private String type; // "VST3" or "CLAP"

	public VSTPlugin(long instance) {
		this.setInstance(instance);
	}

	public String getPath() {
		if (this.path == null && this.isInitialized()) {
			this.path = this.getPath(this.getInstance());
		}
		return this.path;
	}

	public String getType() {
		if (this.type == null && this.isInitialized()) {
			this.type = this.getType(this.getInstance());
		}
		return this.type;
	}

	public List<VSTComponent> getComponents() {
		if (this.components == null && this.isInitialized()) {
			this.components = this.getComponents(this.getInstance());
		}
		return (this.components != null ? this.components : new ArrayList<VSTComponent>());
	}

	// Native methods
	private native String getPath(long instance);

	private native String getType(long instance);

	private native List<VSTComponent> getComponents(long instance);
}