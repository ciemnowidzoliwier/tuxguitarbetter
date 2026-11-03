package app.tuxguitar.midi.synth.impl.vst.jni;

public class VSTInstance extends VSTObject {

	public VSTInstance(long instance) {
		this.setInstance(instance);
	}

	// Native methods for plugin instance management
	public native boolean initialize(double sampleRate, int maxBlockSize);

	public native void terminate();

	public native void processAudio(float[][] inputs, float[][] outputs, int numSamples);

	public native void processMidi(byte[] midiData, int offset, int length, int sampleOffset);

	public native void setParameter(int paramIndex, float value);

	public native float getParameter(int paramIndex);

	public native int getParameterCount();

	public native String getParameterName(int paramIndex);

	public native String getParameterLabel(int paramIndex);

	public native void setProgram(int programIndex);

	public native int getProgram();

	public native int getProgramCount();

	public native String getProgramName(int programIndex);
}