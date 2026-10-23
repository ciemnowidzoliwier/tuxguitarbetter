package app.tuxguitar.player.impl.midiport.nam;

import java.util.ArrayList;
import java.util.List;

import app.tuxguitar.midi.synth.TGAudioProcessorFactory;
import app.tuxguitar.midi.synth.TGSynthExtension;
import app.tuxguitar.midi.synth.TGSynthExtensionPlugin;
import app.tuxguitar.util.TGContext;
import app.tuxguitar.util.plugin.TGPluginException;

public class NamSynthExtension extends TGSynthExtensionPlugin {

    public NamSynthExtension() {
        super();
    }

    public String getModuleId() {
        return NamModule.MODULE_ID;
    }

    @Override
    public List<TGSynthExtension<?>> createExtensions(TGContext context) throws TGPluginException {
        List<TGSynthExtension<?>> extensions = new ArrayList<TGSynthExtension<?>>();
        extensions.add(new TGSynthExtension<TGAudioProcessorFactory>(TGAudioProcessorFactory.class, new NamAudioProcessorFactory(context)));
        return extensions;
    }
}