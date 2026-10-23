package app.tuxguitar.player.impl.midiport.nam;

import app.tuxguitar.midi.synth.TGAudioProcessor;
import app.tuxguitar.midi.synth.TGAudioProcessorFactory;
import app.tuxguitar.util.TGContext;

public class NamAudioProcessorFactory implements TGAudioProcessorFactory {

    private TGContext context;

    public NamAudioProcessorFactory(TGContext context) {
        this.context = context;
    }

    public String getType() {
        return NamModule.AUDIO_TYPE;
    }

    public TGAudioProcessor createProcessor() {
        return new NamAudioProcessor();
    }
}