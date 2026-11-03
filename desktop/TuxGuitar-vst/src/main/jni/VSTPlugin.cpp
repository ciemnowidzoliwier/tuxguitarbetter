#include <jni.h>
#include <string>
#include <vector>
#include "VSTJNICommon.h"

// JNI class names
#define JNI_CLASS_VSTPLUGIN "app/tuxguitar/midi/synth/impl/vst/jni/VSTPlugin"
#define JNI_CLASS_VSTCOMPONENT "app/tuxguitar/midi/synth/impl/vst/jni/VSTComponent"

extern "C" {

// Helper to get plugin info from instance pointer
static void* getPluginInfo(JNIEnv* env, jlong instance) {
    return reinterpret_cast<void*>(instance);
}

// VSTPlugin native methods

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTPlugin_getPath(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getPluginInfo(env, instance);
    if (!ptr) return env->NewStringUTF("");

    // Check if it's VST3 or CLAP by trying both
    // We use a heuristic: if the path ends with .vst3 it's VST3, if .clap it's CLAP
    // Actually, we need to store the type. Let's check the memory layout.
    // For simplicity, we'll use a tag in the first bytes or check the path.
    // Since we stored pointers to VST3PluginInfo and CLAPPluginInfo, we can't easily distinguish.
    // Let's use a different approach: store a wrapper struct.

    // For now, try VST3 first
    VST3PluginInfo* vst3 = static_cast<VST3PluginInfo*>(ptr);
    if (vst3 && !vst3->path.empty() && vst3->path.find(".vst3") != std::string::npos) {
        return env->NewStringUTF(vst3->path.c_str());
    }

    // Try CLAP
    CLAPPluginInfo* clap = static_cast<CLAPPluginInfo*>(ptr);
    if (clap && !clap->path.empty() && clap->path.find(".clap") != std::string::npos) {
        return env->NewStringUTF(clap->path.c_str());
    }

    return env->NewStringUTF("");
}

JNIEXPORT jstring JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTPlugin_getType(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getPluginInfo(env, instance);
    if (!ptr) return env->NewStringUTF("");

    VST3PluginInfo* vst3 = static_cast<VST3PluginInfo*>(ptr);
    if (vst3 && !vst3->path.empty() && vst3->path.find(".vst3") != std::string::npos) {
        return env->NewStringUTF("VST3");
    }

    CLAPPluginInfo* clap = static_cast<CLAPPluginInfo*>(ptr);
    if (clap && !clap->path.empty() && clap->path.find(".clap") != std::string::npos) {
        return env->NewStringUTF("CLAP");
    }

    return env->NewStringUTF("");
}

JNIEXPORT jobjectArray JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTPlugin_getComponents(JNIEnv* env, jobject obj, jlong instance) {
    void* ptr = getPluginInfo(env, instance);
    if (!ptr) return nullptr;

    jclass componentClass = env->FindClass(JNI_CLASS_VSTCOMPONENT);
    if (!componentClass) return nullptr;

    jmethodID componentCtor = env->GetMethodID(componentClass, "<init>", "(J)V");
    if (!componentCtor) return nullptr;

    // Check VST3
    VST3PluginInfo* vst3 = static_cast<VST3PluginInfo*>(ptr);
    if (vst3 && !vst3->path.empty() && vst3->path.find(".vst3") != std::string::npos) {
        size_t count = vst3->components.size();
        jobjectArray result = env->NewObjectArray(count, componentClass, nullptr);
        for (size_t i = 0; i < count; ++i) {
            // Store pointer to component
            jlong compPtr = reinterpret_cast<jlong>(&vst3->components[i]);
            jobject compObj = env->NewObject(componentClass, componentCtor, compPtr);
            env->SetObjectArrayElement(result, i, compObj);
            env->DeleteLocalRef(compObj);
        }
        return result;
    }

    // Check CLAP
    CLAPPluginInfo* clap = static_cast<CLAPPluginInfo*>(ptr);
    if (clap && !clap->path.empty() && clap->path.find(".clap") != std::string::npos) {
        // CLAP plugins have one component per plugin (in this simple model)
        jobjectArray result = env->NewObjectArray(1, componentClass, nullptr);
        jlong compPtr = reinterpret_cast<jlong>(clap);  // Reuse the plugin info as component
        jobject compObj = env->NewObject(componentClass, componentCtor, compPtr);
        env->SetObjectArrayElement(result, 0, compObj);
        env->DeleteLocalRef(compObj);
        return result;
    }

    return nullptr;
}

} // extern "C"