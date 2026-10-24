package app.tuxguitar.player.impl.midiport.nam;

import app.tuxguitar.midi.synth.TGAudioProcessor;
import app.tuxguitar.midi.synth.ui.TGAudioProcessorUI;
import app.tuxguitar.midi.synth.ui.TGAudioProcessorUIFactory;
import app.tuxguitar.midi.synth.ui.TGAudioProcessorUICallback;
import app.tuxguitar.util.TGContext;

public class NamAudioProcessorUIFactory implements TGAudioProcessorUIFactory {

	private TGContext context;

	public NamAudioProcessorUIFactory(TGContext context) {
		this.context = context;
	}

	public String getType() {
		return NamModule.AUDIO_TYPE;
	}

	public TGAudioProcessorUI create(TGAudioProcessor processor, TGAudioProcessorUICallback callback) {
		return new NamAudioProcessorUI(this.context, (NamAudioProcessor) processor, callback);
	}
}