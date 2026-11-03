package app.tuxguitar.player.impl.midiport.vst;

import java.util.ArrayList;
import java.util.List;

import app.tuxguitar.midi.synth.TGAudioProcessor;
import app.tuxguitar.midi.synth.TGAudioProcessorFactory;
import app.tuxguitar.midi.synth.impl.vst.jni.VSTComponent;
import app.tuxguitar.midi.synth.impl.vst.jni.VSTWorld;
import app.tuxguitar.util.TGContext;

public class VSTAudioProcessorFactory implements TGAudioProcessorFactory {

	private TGContext context;
	private VSTWorld vstWorld;
	private String type;

	public VSTAudioProcessorFactory(TGContext context, VSTWorld vstWorld, String type) {
		this.context = context;
		this.vstWorld = vstWorld;
		this.type = type;
	}

	@Override
	public String getType() {
		return this.type;
	}

	@Override
	public List<String> getSupportedPrograms() {
		List<String> programs = new ArrayList<String>();
		if (this.vstWorld != null) {
			for (app.tuxguitar.midi.synth.impl.vst.jni.VSTPlugin plugin : this.vstWorld.getPlugins()) {
				if (plugin.getType().equals(this.getPluginFormat())) {
					for (VSTComponent component : plugin.getComponents()) {
						programs.add(plugin.getPath() + "|" + component.getId());
					}
				}
			}
		}
		return programs;
	}

	@Override
	public TGAudioProcessor createAudioProcessor(String program, TGContext context) {
		return new VSTAudioProcessor(this.context, this.vstWorld, program, this.getPluginFormat());
	}

	private String getPluginFormat() {
		if (this.type.equals(app.tuxguitar.midi.synth.impl.vst.VSTModule.VST3_AUDIO_TYPE)) {
			return "VST3";
		} else if (this.type.equals(app.tuxguitar.midi.synth.impl.vst.VSTModule.CLAP_AUDIO_TYPE)) {
			return "CLAP";
		}
		return "VST3";
	}
}