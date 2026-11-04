// JNI wrapper for FluidSynth on Android

#include <jni.h>
#include <fluidsynth.h>

#include <cstring>
#include <vector>

namespace
{

struct FluidSynthInstance
{
    fluid_settings_t* settings = nullptr;
    fluid_synth_t* synth = nullptr;
    fluid_audio_driver_t* driver = nullptr;
    int sfontId = -1;

    ~FluidSynthInstance()
    {
        close();
    }

    bool open()
    {
        settings = new_fluid_settings();
        if (!settings) return false;

        fluid_settings_setstr(settings, "audio.driver", "opensles");
        fluid_settings_setint(settings, "synth.reverb.active", 0);
        fluid_settings_setint(settings, "synth.chorus.active", 0);
        fluid_settings_setnum(settings, "synth.gain", 1.0);
        fluid_settings_setnum(settings, "synth.sample-rate", 44100.0);
        fluid_settings_setint(settings, "audio.periods", 16);
        fluid_settings_setint(settings, "audio.period-size", 512);

        synth = new_fluid_synth(settings);
        if (!synth) return false;

        driver = new_fluid_audio_driver(settings, synth);
        if (!driver) return false;

        return true;
    }

    void close()
    {
        if (driver) {
            delete_fluid_audio_driver(driver);
            driver = nullptr;
        }
        if (synth) {
            delete_fluid_synth(synth);
            synth = nullptr;
        }
        if (settings) {
            delete_fluid_settings(settings);
            settings = nullptr;
        }
        sfontId = -1;
    }

    int loadFont(const char* path)
    {
        if (!synth) return -1;
        int id = fluid_synth_sfload(synth, path, 1);
        if (id >= 0) {
            sfontId = id;
        }
        return id;
    }

    void unloadFont()
    {
        if (synth && sfontId >= 0) {
            fluid_synth_sfunload(synth, sfontId, 1);
            sfontId = -1;
        }
    }

    void systemReset()
    {
        if (synth) {
            fluid_synth_system_reset(synth);
        }
    }

    void noteOn(int channel, int key, int velocity)
    {
        if (synth) {
            fluid_synth_noteon(synth, channel, key, velocity);
        }
    }

    void noteOff(int channel, int key, int /* velocity */)
    {
        if (synth) {
            // FluidSynth v1.1.11+ fluid_synth_noteoff only takes 3 args (no velocity parameter)
            fluid_synth_noteoff(synth, channel, key);
        }
    }

    void controlChange(int channel, int control, int value)
    {
        if (synth) {
            fluid_synth_cc(synth, channel, control, value);
        }
    }

    void programChange(int channel, int program)
    {
        if (synth) {
            fluid_synth_program_change(synth, channel, program);
        }
    }

    void pitchBend(int channel, int value)
    {
        if (synth) {
            fluid_synth_pitch_bend(synth, channel, value);
        }
    }

    double getDoubleProperty(const char* key)
    {
        if (!settings) return 0.0;
        double value = 0.0;
        fluid_settings_getnum(settings, key, &value);
        return value;
    }

    int getIntegerProperty(const char* key)
    {
        if (!settings) return 0;
        int value = 0;
        fluid_settings_getint(settings, key, &value);
        return value;
    }

    const char* getStringProperty(const char* key)
    {
        if (!settings) return nullptr;
        char* value = nullptr;
        // FluidSynth v1.1.11 has fluid_settings_getstr(settings, name, char**)
        if (fluid_settings_getstr(settings, key, &value) == FLUID_OK) {
            return value;
        }
        return nullptr;
    }

    void setDoubleProperty(const char* key, double value)
    {
        if (settings) {
            fluid_settings_setnum(settings, key, value);
        }
    }

    void setIntegerProperty(const char* key, int value)
    {
        if (settings) {
            fluid_settings_setint(settings, key, value);
        }
    }

    void setStringProperty(const char* key, const char* value)
    {
        if (settings) {
            fluid_settings_setstr(settings, key, value);
        }
    }
};

inline FluidSynthInstance* toInstance(jlong h)
{
    return reinterpret_cast<FluidSynthInstance*>(h);
}

} // namespace

extern "C"
{

JNIEXPORT jlong JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_malloc(JNIEnv*, jclass)
{
    return reinterpret_cast<jlong>(new FluidSynthInstance());
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_free(JNIEnv*, jclass, jlong instance)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        delete inst;
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_open(JNIEnv*, jclass, jlong instance)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->open();
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_close(JNIEnv*, jclass, jlong instance)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->close();
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_loadFont(JNIEnv* env, jclass, jlong instance, jstring path)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (!inst || !path) return;

    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    inst->loadFont(pathStr);
    env->ReleaseStringUTFChars(path, pathStr);
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_unloadFont(JNIEnv*, jclass, jlong instance)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->unloadFont();
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_systemReset(JNIEnv*, jclass, jlong instance)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->systemReset();
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_noteOn(JNIEnv*, jclass, jlong instance, jint channel, jint key, jint velocity)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->noteOn(channel, key, velocity);
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_noteOff(JNIEnv*, jclass, jlong instance, jint channel, jint key, jint velocity)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->noteOff(channel, key, velocity);
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_controlChange(JNIEnv*, jclass, jlong instance, jint channel, jint control, jint value)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->controlChange(channel, control, value);
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_programChange(JNIEnv*, jclass, jlong instance, jint channel, jint program)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->programChange(channel, program);
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_pitchBend(JNIEnv*, jclass, jlong instance, jint channel, jint value)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (inst) {
        inst->pitchBend(channel, value);
    }
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_setDoubleProperty(JNIEnv* env, jclass, jlong instance, jstring key, jdouble value)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (!inst || !key) return;

    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    inst->setDoubleProperty(keyStr, value);
    env->ReleaseStringUTFChars(key, keyStr);
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_setIntegerProperty(JNIEnv* env, jclass, jlong instance, jstring key, jint value)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (!inst || !key) return;

    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    inst->setIntegerProperty(keyStr, value);
    env->ReleaseStringUTFChars(key, keyStr);
}

JNIEXPORT void JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_setStringProperty(JNIEnv* env, jclass, jlong instance, jstring key, jstring value)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (!inst || !key) return;

    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    const char* valueStr = value ? env->GetStringUTFChars(value, nullptr) : nullptr;
    inst->setStringProperty(keyStr, valueStr);
    if (valueStr) env->ReleaseStringUTFChars(value, valueStr);
    env->ReleaseStringUTFChars(key, keyStr);
}

JNIEXPORT jdouble JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_getDoubleProperty(JNIEnv* env, jclass, jlong instance, jstring key)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (!inst || !key) return 0.0;

    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    double value = inst->getDoubleProperty(keyStr);
    env->ReleaseStringUTFChars(key, keyStr);
    return value;
}

JNIEXPORT jint JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_getIntegerProperty(JNIEnv* env, jclass, jlong instance, jstring key)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (!inst || !key) return 0;

    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    int value = inst->getIntegerProperty(keyStr);
    env->ReleaseStringUTFChars(key, keyStr);
    return value;
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_player_impl_midiport_fluidsynth_MidiSynth_getStringProperty(JNIEnv* env, jclass, jlong instance, jstring key)
{
    FluidSynthInstance* inst = toInstance(instance);
    if (!inst || !key) return nullptr;

    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    const char* value = inst->getStringProperty(keyStr);
    env->ReleaseStringUTFChars(key, keyStr);

    if (value) {
        return env->NewStringUTF(value);
    }
    return nullptr;
}

} // extern "C"