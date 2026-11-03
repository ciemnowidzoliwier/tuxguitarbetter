package app.tuxguitar.midi.synth.impl.vst.jni;

public class VSTComponent extends VSTObject {

	private String name;
	private String id;
	private String category;
	private String vendor;
	private String subCategories;
	private boolean isInstrument;

	public VSTComponent(long instance) {
		this.setInstance(instance);
	}

	public String getName() {
		if (this.name == null && this.isInitialized()) {
			this.name = this.getName(this.getInstance());
		}
		return this.name;
	}

	public String getId() {
		if (this.id == null && this.isInitialized()) {
			this.id = this.getId(this.getInstance());
		}
		return this.id;
	}

	public String getCategory() {
		if (this.category == null && this.isInitialized()) {
			this.category = this.getCategory(this.getInstance());
		}
		return this.category;
	}

	public String getVendor() {
		if (this.vendor == null && this.isInitialized()) {
			this.vendor = this.getVendor(this.getInstance());
		}
		return this.vendor;
	}

	public String getSubCategories() {
		if (this.subCategories == null && this.isInitialized()) {
			this.subCategories = this.getSubCategories(this.getInstance());
		}
		return this.subCategories;
	}

	public boolean isInstrument() {
		if (!this.isInitialized()) {
			return false;
		}
		if (this.isInstrument == false) {
			this.isInstrument = this.checkIsInstrument(this.getInstance());
		}
		return this.isInstrument;
	}

	// Native methods
	private native String getName(long instance);

	private native String getId(long instance);

	private native String getCategory(long instance);

	private native String getVendor(long instance);

	private native String getSubCategories(long instance);

	private native boolean checkIsInstrument(long instance);
}