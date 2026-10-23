package app.tuxguitar.player.impl.midiport.nam;

import java.util.Map;

import app.tuxguitar.midi.synth.TGAudioBuffer;
import app.tuxguitar.midi.synth.TGAudioProcessor;

public class NamAudioProcessor implements TGAudioProcessor {

    // M2: here comes nam_process() via JNI. Spike marker: -6dB.
    private static final float SPIKE_GAIN = 0.5f;

    private float[][] samples;

    public NamAudioProcessor() {
        this.samples = new float[TGAudioBuffer.CHANNELS][TGAudioBuffer.BUFFER_SIZE / 2];
        System.out.println("[TG-NAM] NamAudioProcessor created (spike: pass-through x0.5)");
    }

    public void open() {
        // M2: open NAM instance
    }

    public void close() {
        // M2: close NAM instance
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
        for (int c = 0; c < this.samples.length; c++) {
            float[] channel = this.samples[c];
            for (int i = 0; i < channel.length; i++) {
                channel[i] = channel[i] * SPIKE_GAIN;
            }
        }
        buffer.write(this.samples);
    }

    public void storeParameters(Map<String, String> parameters) {
        // M3: store .nam model path, gain, bypass
    }

    public void restoreParameters(Map<String, String> parameters) {
        // M3: restore .nam model path, gain, bypass
    }
}