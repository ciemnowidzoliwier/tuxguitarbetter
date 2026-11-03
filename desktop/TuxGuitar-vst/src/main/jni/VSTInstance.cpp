#include <jni.h>
#include <string>
#include <vector>
#include <memory>
#include "VSTJNICommon.h"

// VST3 SDK includes
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/module.h"

// CLAP includes
#include <clap/entry.h>
#include <clap/factory/plugin-factory.h>
#include <clap/ext/params.h>
#include <clap/ext/audio-ports.h>
#include <clap/ext/note-ports.h>
#include <clap/ext/process.h>

// JNI class name
#define JNI_CLASS_VSTINSTANCE "app/tuxguitar/midi/synth/impl/vst/jni/VSTInstance"

// VST3 Instance wrapper
struct VST3Instance {
    std::shared_ptr<VST3::Hosting::Module> module;
    Steinberg::Vst::IComponent* component = nullptr;
    Steinberg::Vst::IAudioProcessor* processor = nullptr;
    Steinberg::Vst::IEditController* controller = nullptr;
    Steinberg::Vst::ProcessSetup setup;
    std::vector<Steinberg::Vst::ParamID> paramIds;
};

// CLAP Instance wrapper
struct CLAPInstance {
    const clap_plugin_entry_t* entry = nullptr;
    const clap_plugin_factory_t* factory = nullptr;
    const clap_plugin_t* plugin = nullptr;
    void* handle = nullptr;
    clap_process_t process = {};
    std::vector<clap_param_info_t> paramInfos;
};

// Union for both types
struct VSTInstanceData {
    enum Type { NONE, VST3, CLAP } type = NONE;
    union {
        VST3Instance vst3;
        CLAPInstance clap;
    };
    std::string pluginPath;
    std::string componentId;

    ~VSTInstanceData() {
        if (type == VST3) {
            if (vst3.processor) vst3.processor->release();
            if (vst3.controller) vst3.controller->release();
            if (vst3.component) vst3.component->release();
        } else if (type == CLAP) {
            if (clap.plugin && clap.entry) {
                clap.entry->deinit();
            }
            if (clap.handle) {
                dlclose(clap.handle);
            }
        }
    }
};

extern "C" {

// Helper to get instance data
static VSTInstanceData* getInstanceData(JNIEnv* env, jlong instance) {
    return reinterpret_cast<VSTInstanceData*>(instance);
}

// Create VST3 instance
static VSTInstanceData* createVST3Instance(const std::string& pluginPath, const std::string& componentId) {
    auto data = new VSTInstanceData();
    data->type = VSTInstanceData::VST3;
    data->pluginPath = pluginPath;
    data->componentId = componentId;

    std::string error;
    data->vst3.module = VST3::Hosting::Module::create(pluginPath, error);
    if (!data->vst3.module) {
        delete data;
        return nullptr;
    }

    // Find the component by ID
    const auto& classInfos = data->vst3.module->getFactory().classInfos();
    Steinberg::Vst::TUID targetUID;
    // Parse componentId (which is the TUID string) back to TUID
    // componentId format: 8+8+8+8 hex chars
    if (componentId.length() == 32) {
        sscanf(componentId.c_str(), "%08X%08X%08X%08X",
            &targetUID[0], &targetUID[1], &targetUID[2], &targetUID[3]);
    } else {
        delete data;
        return nullptr;
    }

    for (const auto& classInfo : classInfos) {
        if (classInfo.ID() == targetUID) {
            // Create component
            Steinberg::FUnknown* unknown = nullptr;
            if (data->vst3.module->getFactory().createInstance(classInfo.ID(), Steinberg::Vst::IComponent::iid, (void**)&unknown) == Steinberg::kResultOk) {
                data->vst3.component = Steinberg::Vst::IComponent::from(unknown);
            }
            break;
        }
    }

    if (!data->vst3.component) {
        delete data;
        return nullptr;
    }

    // Initialize component
    if (data->vst3.component->initialize(nullptr) != Steinberg::kResultOk) {
        delete data;
        return nullptr;
    }

    // Get audio processor
    Steinberg::FUnknown* processorUnknown = nullptr;
    if (data->vst3.component->getControllerClassId().isValid()) {
        // Has controller, get processor from component
        data->vst3.component->queryInterface(Steinberg::Vst::IAudioProcessor::iid, (void**)&data->vst3.processor);
    } else {
        // No controller, component is the processor
        data->vst3.component->queryInterface(Steinberg::Vst::IAudioProcessor::iid, (void**)&data->vst3.processor);
    }

    // Get controller
    data->vst3.component->queryInterface(Steinberg::Vst::IEditController::iid, (void**)&data->vst3.controller);

    if (!data->vst3.processor) {
        delete data;
        return nullptr;
    }

    // Setup process
    data->vst3.setup.processMode = Steinberg::Vst::ProcessModes::kRealtime;
    data->vst3.setup.symbolicSampleSize = Steinberg::Vst::SymbolicSampleSizes::kSample32;
    data->vst3.setup.maxSamplesPerBlock = 512;
    data->vst3.setup.sampleRate = 44100.0;
    data->vst3.processor->setupProcessing(data->vst3.setup);

    // Collect parameter IDs
    if (data->vst3.controller) {
        int32_t paramCount = data->vst3.controller->getParameterCount();
        for (int32_t i = 0; i < paramCount; ++i) {
            Steinberg::Vst::ParameterInfo paramInfo;
            if (data->vst3.controller->getParameterInfo(i, paramInfo) == Steinberg::kResultOk) {
                data->vst3.paramIds.push_back(paramInfo.id);
            }
        }
    }

    return data;
}

// Create CLAP instance
static VSTInstanceData* createCLAPInstance(const std::string& pluginPath, const std::string& componentId) {
    void* handle = dlopen(pluginPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) return nullptr;

    using clap_entry_fn = const clap_plugin_entry_t* (*)();
    clap_entry_fn entryFn = (clap_entry_fn)dlsym(handle, "clap_entry");
    if (!entryFn) {
        dlclose(handle);
        return nullptr;
    }

    const clap_plugin_entry_t* entry = entryFn();
    if (!entry || entry->clap_version.major != 1) {
        dlclose(handle);
        return nullptr;
    }

    if (!entry->init(pluginPath.c_str())) {
        dlclose(handle);
        return nullptr;
    }

    const clap_plugin_factory_t* factory = (const clap_plugin_factory_t*)
        entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    if (!factory) {
        entry->deinit();
        dlclose(handle);
        return nullptr;
    }

    uint32_t count = factory->get_plugin_count(factory);
    for (uint32_t i = 0; i < count; ++i) {
        const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, i);
        if (!desc) continue;

        if (desc->id && std::string(desc->id) == componentId) {
            const clap_plugin_t* plugin = factory->create_plugin(factory, desc->id, nullptr);
            if (!plugin) continue;

            auto data = new VSTInstanceData();
            data->type = VSTInstanceData::CLAP;
            data->pluginPath = pluginPath;
            data->componentId = componentId;
            data->clap.entry = entry;
            data->clap.factory = factory;
            data->clap.plugin = plugin;
            data->clap.handle = handle;

            // Initialize plugin
            if (plugin->init(plugin)) {
                // Collect parameter info
                const clap_plugin_params_t* paramsExt = (const clap_plugin_params_t*)
                    plugin->get_extension(plugin, CLAP_EXT_PARAMS);
                if (paramsExt) {
                    uint32_t paramCount = paramsExt->count(plugin);
                    for (uint32_t j = 0; j < paramCount; ++j) {
                        clap_param_info_t paramInfo;
                        if (paramsExt->get_info(plugin, j, &paramInfo)) {
                            data->clap.paramInfos.push_back(paramInfo);
                        }
                    }
                }
                return data;
            }

            delete data;
            return nullptr;
        }
    }

    entry->deinit();
    dlclose(handle);
    return nullptr;
}

// JNI native methods

JNIEXPORT jlong JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_initialize__JJDDI(JNIEnv* env, jobject obj, jlong instance, jdouble sampleRate, jint maxBlockSize) {
    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data) return JNI_FALSE;

    if (data->type == VSTInstanceData::VST3) {
        data->vst3.setup.sampleRate = sampleRate;
        data->vst3.setup.maxSamplesPerBlock = maxBlockSize;
        if (data->vst3.processor) {
            return data->vst3.processor->setupProcessing(data->vst3.setup) == Steinberg::kResultOk ? JNI_TRUE : JNI_FALSE;
        }
    } else if (data->type == VSTInstanceData::CLAP) {
        // CLAP doesn't have explicit setup, but we can store the info
        // The process call will use these values
    }
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_terminate(JNIEnv* env, jobject obj, jlong instance) {
    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data) return;

    if (data->type == VSTInstanceData::VST3) {
        if (data->vst3.component) {
            data->vst3.component->terminate();
        }
    } else if (data->type == VSTInstanceData::CLAP) {
        if (data->clap.plugin && data->clap.entry) {
            data->clap.entry->deinit();
        }
    }
    // The destructor will clean up
    delete data;
}

JNIEXPORT void JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_processAudio(
    JNIEnv* env, jobject obj, jlong instance,
    jobjectArray inputs, jobjectArray outputs, jint numSamples) {

    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data || numSamples <= 0) return;

    // Convert Java float[][] to C++ arrays
    int numInputs = env->GetArrayLength(inputs);
    int numOutputs = env->GetArrayLength(outputs);

    std::vector<float*> inputBuffers(numInputs);
    std::vector<float*> outputBuffers(numOutputs);

    for (int i = 0; i < numInputs; ++i) {
        jfloatArray arr = (jfloatArray)env->GetObjectArrayElement(inputs, i);
        inputBuffers[i] = env->GetFloatArrayElements(arr, nullptr);
        env->DeleteLocalRef(arr);
    }
    for (int i = 0; i < numOutputs; ++i) {
        jfloatArray arr = (jfloatArray)env->GetObjectArrayElement(outputs, i);
        outputBuffers[i] = env->GetFloatArrayElements(arr, nullptr);
        env->DeleteLocalRef(arr);
    }

    if (data->type == VSTInstanceData::VST3 && data->vst3.processor) {
        Steinberg::Vst::ProcessData processData;
        processData.numInputs = numInputs;
        processData.numOutputs = numOutputs;
        processData.numSamples = numSamples;
        processData.sampleRate = data->vst3.setup.sampleRate;
        processData.processMode = Steinberg::Vst::ProcessModes::kRealtime;

        // Setup input buffers
        std::vector<Steinberg::Vst::AudioBusBuffers> inputBuses(numInputs);
        for (int i = 0; i < numInputs; ++i) {
            inputBuses[i].numChannels = 1;
            inputBuses[i].channelBuffers32 = &inputBuffers[i];
            inputBuses[i].silenceFlags = 0;
        }
        processData.inputs = inputBuses.data();

        // Setup output buffers
        std::vector<Steinberg::Vst::AudioBusBuffers> outputBuses(numOutputs);
        for (int i = 0; i < numOutputs; ++i) {
            outputBuses[i].numChannels = 1;
            outputBuses[i].channelBuffers32 = &outputBuffers[i];
            outputBuses[i].silenceFlags = 0;
        }
        processData.outputs = outputBuses.data();

        data->vst3.processor->process(processData);
    } else if (data->type == VSTInstanceData::CLAP && data->clap.plugin) {
        const clap_plugin_audio_ports_t* audioPorts = (const clap_plugin_audio_ports_t*)
            data->clap.plugin->get_extension(data->clap.plugin, CLAP_EXT_AUDIO_PORTS);
        const clap_plugin_process_t* processExt = (const clap_plugin_process_t*)
            data->clap.plugin->get_extension(data->clap.plugin, CLAP_EXT_PROCESS);

        if (processExt) {
            clap_process_t process = {};
            process.audio_inputs = nullptr;  // Simplified
            process.audio_outputs = nullptr;
            process.audio_inputs_count = numInputs;
            process.audio_outputs_count = numOutputs;
            process.frames_count = numSamples;
            process.transport = nullptr;

            processExt->process(data->clap.plugin, &process);
        }
    }

    // Release arrays
    for (int i = 0; i < numInputs; ++i) {
        jfloatArray arr = (jfloatArray)env->GetObjectArrayElement(inputs, i);
        env->ReleaseFloatArrayElements(arr, inputBuffers[i], 0);
        env->DeleteLocalRef(arr);
    }
    for (int i = 0; i < numOutputs; ++i) {
        jfloatArray arr = (jfloatArray)env->GetObjectArrayElement(outputs, i);
        env->ReleaseFloatArrayElements(arr, outputBuffers[i], 0);
        env->DeleteLocalRef(arr);
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_processMidi(
    JNIEnv* env, jobject obj, jlong instance,
    jbyteArray dataArray, jint offset, jint length, jint sampleOffset) {

    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data || length <= 0) return;

    jbyte* midiData = env->GetByteArrayElements(dataArray, nullptr);

    if (data->type == VSTInstanceData::VST3 && data->vst3.component) {
        // VST3 MIDI processing via IComponent
        // This is simplified - real implementation needs proper event list
    } else if (data->type == VSTInstanceData::CLAP && data->clap.plugin) {
        const clap_plugin_note_ports_t* notePorts = (const clap_plugin_note_ports_t*)
            data->clap.plugin->get_extension(data->clap.plugin, CLAP_EXT_NOTE_PORTS);
        const clap_plugin_process_t* processExt = (const clap_plugin_process_t*)
            data->clap.plugin->get_extension(data->clap.plugin, CLAP_EXT_PROCESS);

        // CLAP MIDI processing would go here
    }

    env->ReleaseByteArrayElements(dataArray, midiData, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_setParameter(
    JNIEnv* env, jobject obj, jlong instance, jint paramIndex, jfloat value) {

    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data) return;

    if (data->type == VSTInstanceData::VST3 && data->vst3.controller && paramIndex < (int)data->vst3.paramIds.size()) {
        data->vst3.controller->setParamNormalized(data->vst3.paramIds[paramIndex], value);
    } else if (data->type == VSTInstanceData::CLAP && data->clap.plugin) {
        const clap_plugin_params_t* paramsExt = (const clap_plugin_params_t*)
            data->clap.plugin->get_extension(data->clap.plugin, CLAP_EXT_PARAMS);
        if (paramsExt && paramIndex < (int)data->clap.paramInfos.size()) {
            paramsExt->set_value(data->clap.plugin, data->clap.paramInfos[paramIndex].id, value);
        }
    }
}

JNIEXPORT jfloat JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_getParameter(
    JNIEnv* env, jobject obj, jlong instance, jint paramIndex) {

    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data) return 0.0f;

    if (data->type == VSTInstanceData::VST3 && data->vst3.controller && paramIndex < (int)data->vst3.paramIds.size()) {
        return data->vst3.controller->getParamNormalized(data->vst3.paramIds[paramIndex]);
    } else if (data->type == VSTInstanceData::CLAP && data->clap.plugin) {
        const clap_plugin_params_t* paramsExt = (const clap_plugin_params_t*)
            data->clap.plugin->get_extension(data->clap.plugin, CLAP_EXT_PARAMS);
        if (paramsExt && paramIndex < (int)data->clap.paramInfos.size()) {
            double value;
            if (paramsExt->get_value(data->clap.plugin, data->clap.paramInfos[paramIndex].id, &value)) {
                return (jfloat)value;
            }
        }
    }
    return 0.0f;
}

JNIEXPORT jint JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_getParameterCount(
    JNIEnv* env, jobject obj, jlong instance) {

    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data) return 0;

    if (data->type == VSTInstanceData::VST3) {
        return data->vst3.paramIds.size();
    } else if (data->type == VSTInstanceData::CLAP) {
        return data->clap.paramInfos.size();
    }
    return 0;
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_getParameterName(
    JNIEnv* env, jobject obj, jlong instance, jint paramIndex) {

    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data) return env->NewStringUTF("");

    if (data->type == VSTInstanceData::VST3 && data->vst3.controller && paramIndex < (int)data->vst3.paramIds.size()) {
        Steinberg::Vst::ParameterInfo paramInfo;
        if (data->vst3.controller->getParameterInfo(paramIndex, paramInfo) == Steinberg::kResultOk) {
            return env->NewStringUTF(paramInfo.title);
        }
    } else if (data->type == VSTInstanceData::CLAP && data->clap.plugin) {
        if (paramIndex < (int)data->clap.paramInfos.size()) {
            return env->NewStringUTF(data->clap.paramInfos[paramIndex].name);
        }
    }
    return env->NewStringUTF("");
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_getParameterLabel(
    JNIEnv* env, jobject obj, jlong instance, jint paramIndex) {

    VSTInstanceData* data = getInstanceData(env, instance);
    if (!data) return env->NewStringUTF("");

    if (data->type == VSTInstanceData::VST3 && data->vst3.controller && paramIndex < (int)data->vst3.paramIds.size()) {
        Steinberg::Vst::ParameterInfo paramInfo;
        if (data->vst3.controller->getParameterInfo(paramIndex, paramInfo) == Steinberg::kResultOk) {
            return env->NewStringUTF(paramInfo.units);
        }
    } else if (data->type == VSTInstanceData::CLAP && data->clap.plugin) {
        if (paramIndex < (int)data->clap.paramInfos.size()) {
            return env->NewStringUTF(data->clap.paramInfos[paramIndex].unit);
        }
    }
    return env->NewStringUTF("");
}

JNIEXPORT void JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_setProgram(
    JNIEnv* env, jobject obj, jlong instance, jint program) {
    // Not implemented for VST3/CLAP in this basic version
}

JNIEXPORT jint JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_getProgram(
    JNIEnv* env, jobject obj, jlong instance) {
    return 0;
}

JNIEXPORT jint JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_getProgramCount(
    JNIEnv* env, jobject obj, jlong instance) {
    return 0;
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTInstance_getProgramName(
    JNIEnv* env, jobject obj, jlong instance, jint program) {
    return env->NewStringUTF("");
}

// Factory method to create instance
JNIEXPORT jlong JNICALL Java_app_tuxguitar_player_impl_midiport_vst_VSTMidiProcessor_createInstance(
    JNIEnv* env, jobject obj, jstring pluginPath, jstring componentId) {

    const char* pathStr = env->GetStringUTFChars(pluginPath, nullptr);
    const char* compStr = env->GetStringUTFChars(componentId, nullptr);

    std::string path(pathStr);
    std::string compId(compStr);

    env->ReleaseStringUTFChars(pluginPath, pathStr);
    env->ReleaseStringUTFChars(componentId, compStr);

    VSTInstanceData* data = nullptr;

    if (path.find(".vst3") != std::string::npos) {
        data = createVST3Instance(path, compId);
    } else if (path.find(".clap") != std::string::npos) {
        data = createCLAPInstance(path, compId);
    }

    return reinterpret_cast<jlong>(data);
}

// Same for audio processor
JNIEXPORT jlong JNICALL Java_app_tuxguitar_player_impl_midiport_vst_VSTAudioProcessor_createInstance(
    JNIEnv* env, jobject obj, jstring pluginPath, jstring componentId) {
    return Java_app_tuxguitar_player_impl_midiport_vst_VSTMidiProcessor_createInstance(env, obj, pluginPath, componentId);
}

} // extern "C"