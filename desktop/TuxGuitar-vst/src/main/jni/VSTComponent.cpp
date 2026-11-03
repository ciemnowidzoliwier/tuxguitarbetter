#include <jni.h>
#include <string>
#include "VSTJNICommon.h"

// JNI class names
#define JNI_CLASS_VSTCOMPONENT "app/tuxguitar/midi/synth/impl/vst/jni/VSTComponent"

extern "C" {

// Helper to get component info from instance pointer
static void* getComponentInfo(JNIEnv* env, jlong instance) {
    return reinterpret_cast<void*>(instance);
}

// VSTComponent native methods

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTComponent_getName(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getComponentInfo(env, instance);
    if (!ptr) return env->NewStringUTF("");

    // Check if it's VST3 component
    VST3ComponentInfo* vst3 = static_cast<VST3ComponentInfo*>(ptr);
    if (vst3 && !vst3->name.empty()) {
        return env->NewStringUTF(vst3->name.c_str());
    }

    // Check if it's CLAP plugin (stored as component)
    CLAPPluginInfo* clap = static_cast<CLAPPluginInfo*>(ptr);
    if (clap && !clap->name.empty()) {
        return env->NewStringUTF(clap->name.c_str());
    }

    return env->NewStringUTF("");
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTComponent_getId(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getComponentInfo(env, instance);
    if (!ptr) return env->NewStringUTF("");

    VST3ComponentInfo* vst3 = static_cast<VST3ComponentInfo*>(ptr);
    if (vst3 && !vst3->id.empty()) {
        return env->NewStringUTF(vst3->id.c_str());
    }

    CLAPPluginInfo* clap = static_cast<CLAPPluginInfo*>(ptr);
    if (clap && !clap->id.empty()) {
        return env->NewStringUTF(clap->id.c_str());
    }

    return env->NewStringUTF("");
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTComponent_getCategory(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getComponentInfo(env, instance);
    if (!ptr) return env->NewStringUTF("");

    VST3ComponentInfo* vst3 = static_cast<VST3ComponentInfo*>(ptr);
    if (vst3 && !vst3->category.empty()) {
        return env->NewStringUTF(vst3->category.c_str());
    }

    CLAPPluginInfo* clap = static_cast<CLAPPluginInfo*>(ptr);
    if (clap) {
        // CLAP doesn't have category in the same way, use features
        return env->NewStringUTF(clap->isInstrument ? "Instrument" : "Effect");
    }

    return env->NewStringUTF("");
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTComponent_getVendor(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getComponentInfo(env, instance);
    if (!ptr) return env->NewStringUTF("");

    VST3ComponentInfo* vst3 = static_cast<VST3ComponentInfo*>(ptr);
    if (vst3 && !vst3->vendor.empty()) {
        return env->NewStringUTF(vst3->vendor.c_str());
    }

    CLAPPluginInfo* clap = static_cast<CLAPPluginInfo*>(ptr);
    if (clap && !clap->vendor.empty()) {
        return env->NewStringUTF(clap->vendor.c_str());
    }

    return env->NewStringUTF("");
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTComponent_getSubCategories(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getComponentInfo(env, instance);
    if (!ptr) return env->NewStringUTF("");

    VST3ComponentInfo* vst3 = static_cast<VST3ComponentInfo*>(ptr);
    if (vst3 && !vst3->subCategories.empty()) {
        return env->NewStringUTF(vst3->subCategories.c_str());
    }

    return env->NewStringUTF("");
}

JNIEXPORT jboolean JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTComponent_checkIsInstrument(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getComponentInfo(env, instance);
    if (!ptr) return JNI_FALSE;

    VST3ComponentInfo* vst3 = static_cast<VST3ComponentInfo*>(ptr);
    if (vst3) {
        return vst3->isInstrument ? JNI_TRUE : JNI_FALSE;
    }

    CLAPPluginInfo* clap = static_cast<CLAPPluginInfo*>(ptr);
    if (clap) {
        return clap->isInstrument ? JNI_TRUE : JNI_FALSE;
    }

    return JNI_FALSE;
}

} // extern "C"