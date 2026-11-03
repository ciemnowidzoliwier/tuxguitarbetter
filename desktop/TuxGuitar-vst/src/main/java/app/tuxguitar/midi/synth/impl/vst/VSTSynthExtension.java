package app.tuxguitar.midi.synth.impl.vst;

import java.util.ArrayList;
import java.util.List;

import app.tuxguitar.midi.synth.TGAudioProcessorFactory;
import app.tuxguitar.midi.synth.TGMidiProcessorFactory;
import app.tuxguitar.midi.synth.TGSynthExtension;
import app.tuxguitar.midi.synth.TGSynthExtensionPlugin;
import app.tuxguitar.midi.synth.ui.TGAudioProcessorUIFactory;
import app.tuxguitar.player.impl.midiport.vst.VSTMidiProcessorFactory;
import app.tuxguitar.player.impl.midiport.vst.VSTAudioProcessorFactory;
import app.tuxguitar.player.impl.midiport.vst.VSTAudioProcessorUIFactory;
import app.tuxguitar.util.TGContext;
import app.tuxguitar.util.plugin.TGPluginException;

public class VSTSynthExtension extends TGSynthExtensionPlugin {

	public VSTSynthExtension() {
		super();
	}

	public String getModuleId() {
		return VSTModule.MODULE_ID;
	}

	@Override
	public List<TGSynthExtension<?>> createExtensions(TGContext context) throws TGPluginException {
		List<TGSynthExtension<?>> extensions = new ArrayList<TGSynthExtension<?>>();
		try {
			// Initialize VST world to scan for plugins
			app.tuxguitar.midi.synth.impl.vst.jni.VSTWorld vstWorld = new app.tuxguitar.midi.synth.impl.vst.jni.VSTWorld();
			vstWorld.getPlugins();

			// Register MIDI processor factory for VST3 instruments
			extensions.add(new TGSynthExtension<TGMidiProcessorFactory>(
				TGMidiProcessorFactory.class,
				new VSTMidiProcessorFactory(context, vstWorld, VSTModule.VST3_MIDI_TYPE)
			));

			// Register MIDI processor factory for CLAP instruments
			extensions.add(new TGSynthExtension<TGMidiProcessorFactory>(
				TGMidiProcessorFactory.class,
				new VSTMidiProcessorFactory(context, vstWorld, VSTModule.CLAP_MIDI_TYPE)
			));

			// Register audio processor factory for VST3
			extensions.add(new TGSynthExtension<TGAudioProcessorFactory>(
				TGAudioProcessorFactory.class,
				new VSTAudioProcessorFactory(context, vstWorld, VSTModule.VST3_AUDIO_TYPE)
			));

			// Register audio processor factory for CLAP
			extensions.add(new TGSynthExtension<TGAudioProcessorFactory>(
				TGAudioProcessorFactory.class,
				new VSTAudioProcessorFactory(context, vstWorld, VSTModule.CLAP_AUDIO_TYPE)
			));

			// Register UI factory for VST3
			extensions.add(new TGSynthExtension<TGAudioProcessorUIFactory>(
				TGAudioProcessorUIFactory.class,
				new VSTAudioProcessorUIFactory(context, vstWorld, VSTModule.VST3_AUDIO_TYPE)
			));

			// Register UI factory for CLAP
			extensions.add(new TGSynthExtension<TGAudioProcessorUIFactory>(
				TGAudioProcessorUIFactory.class,
				new VSTAudioProcessorUIFactory(context, vstWorld, VSTModule.CLAP_AUDIO_TYPE)
			));

		} catch (Throwable e) {
			e.printStackTrace();
		}
		return extensions;
	}
}