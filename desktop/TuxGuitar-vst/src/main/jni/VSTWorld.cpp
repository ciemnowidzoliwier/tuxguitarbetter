#include "VSTJNICommon.h"
#include <jni.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <filesystem>
#include <dlfcn.h>
#include <cstdio>

// VST3 SDK includes
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/classinfo.h"
#include "public.sdk/source/vst/hosting/stringconvert.h"

// CLAP includes
#include <clap/entry.h>
#include <clap/factory/plugin-factory.h>

// JNI class names
#define JNI_CLASS_VSTWORLD "app/tuxguitar/midi/synth/impl/vst/jni/VSTWorld"
#define JNI_CLASS_VSTPLUGIN "app/tuxguitar/midi/synth/impl/vst/jni/VSTPlugin"
#define JNI_CLASS_VSTCOMPONENT "app/tuxguitar/midi/synth/impl/vst/jni/VSTComponent"

namespace fs = std::filesystem;

// Forward declarations for JNI helper functions
static jclass findClass(JNIEnv* env, const char* name);
static jmethodID getMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig);
static jstring toJString(JNIEnv* env, const std::string& str);

// VST3 plugin info
struct VST3ComponentInfo {
    std::string name;
    std::string id;  // TUID as string
    std::string category;
    std::string vendor;
    std::string subCategories;
    bool isInstrument = false;
};

struct VST3PluginInfo {
    std::string path;
    std::vector<VST3ComponentInfo> components;
};

// CLAP plugin info
struct CLAPPluginInfo {
    std::string path;
    std::string name;
    std::string id;
    std::string vendor;
    bool isInstrument = false;
    void* handle = nullptr;
    const clap_plugin_factory_t* factory = nullptr;
    const clap_plugin_entry_t* entry = nullptr;
};

struct VSTWorldData {
    std::vector<VST3PluginInfo> vst3Plugins;
    std::vector<CLAPPluginInfo> clapPlugins;
    std::mutex mutex;
};

static VSTWorldData* getVSTWorldData(JNIEnv* env, jlong instance) {
    return reinterpret_cast<VSTWorldData*>(instance);
}

// Helper: convert Steinberg::Vst::TUID to string
static std::string tuidToString(const Steinberg::Vst::TUID& tuid) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%08X%08X%08X%08X",
        tuid[0], tuid[1], tuid[2], tuid[3]);
    return std::string(buffer);
}

// Helper: check if category indicates an instrument
static bool isInstrumentCategory(const std::string& category) {
    static const std::vector<std::string> instrumentCategories = {
        "Instrument", "Synth", "Generator", "Sampler", "Drum"
    };
    for (const auto& cat : instrumentCategories) {
        if (category.find(cat) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Scan for VST3 plugins in standard locations
static void scanVST3Plugins(VSTWorldData* data) {
    std::vector<std::string> searchPaths = {
        "/usr/lib/vst3",
        "/usr/local/lib/vst3",
        "/opt/vst3",
        "/usr/lib/lv2",  // Sometimes VST3 in LV2 dirs
    };

    // Add user paths
    const char* home = getenv("HOME");
    if (home) {
        searchPaths.push_back(std::string(home) + "/.vst3");
        searchPaths.push_back(std::string(home) + "/.local/lib/vst3");
    }

    // VST3_HOST_PATH env var
    if (const char* vst3Path = getenv("VST3_PATH")) {
        searchPaths.insert(searchPaths.begin(), vst3Path);
    }

    for (const auto& path : searchPaths) {
        try {
            if (!fs::exists(path) || !fs::is_directory(path)) continue;

            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (!entry.is_regular_file()) continue;
                if (entry.path().extension() != ".vst3") continue;

                std::string pluginPath = entry.path().string();
                std::string error;
                auto module = VST3::Hosting::Module::create(pluginPath, error);
                if (!module) continue;

                VST3PluginInfo pluginInfo;
                pluginInfo.path = pluginPath;

                const auto& classInfos = module->getFactory().classInfos();
                for (const auto& classInfo : classInfos) {
                    VST3ComponentInfo compInfo;
                    compInfo.name = classInfo.name().c_str();
                    compInfo.id = tuidToString(classInfo.ID());
                    compInfo.category = classInfo.category().c_str();
                    compInfo.vendor = classInfo.vendor().c_str();
                    compInfo.subCategories = classInfo.subCategoriesString().c_str();
                    compInfo.isInstrument = isInstrumentCategory(compInfo.category);

                    pluginInfo.components.push_back(std::move(compInfo));
                }

                if (!pluginInfo.components.empty()) {
                    std::lock_guard<std::mutex> lock(data->mutex);
                    data->vst3Plugins.push_back(std::move(pluginInfo));
                }
            }
        } catch (const std::exception&) {
            // Ignore errors for individual paths
        }
    }
}

// Scan for CLAP plugins
static void scanCLAPPlugins(VSTWorldData* data) {
    std::vector<std::string> searchPaths = {
        "/usr/lib/clap",
        "/usr/local/lib/clap",
        "/opt/clap",
    };

    // Add user paths
    const char* home = getenv("HOME");
    if (home) {
        searchPaths.push_back(std::string(home) + "/.clap");
        searchPaths.push_back(std::string(home) + "/.local/lib/clap");
    }

    // CLAP_PATH env var (semicolon-separated)
    if (const char* clapPath = getenv("CLAP_PATH")) {
        std::string paths(clapPath);
        size_t start = 0;
        while (start < paths.size()) {
            size_t end = paths.find(';', start);
            if (end == std::string::npos) end = paths.size();
            if (end > start) {
                searchPaths.insert(searchPaths.begin(), paths.substr(start, end - start));
            }
            start = end + 1;
        }
    }

    for (const auto& path : searchPaths) {
        try {
            if (!fs::exists(path) || !fs::is_directory(path)) continue;

            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (!entry.is_regular_file()) continue;
                if (entry.path().extension() != ".clap") continue;

                std::string pluginPath = entry.path().string();
                void* handle = dlopen(pluginPath.c_str(), RTLD_NOW | RTLD_LOCAL);
                if (!handle) continue;

                // Get entry point
                using clap_entry_fn = const clap_plugin_entry_t* (*)();
                clap_entry_fn entryFn = (clap_entry_fn)dlsym(handle, "clap_entry");
                if (!entryFn) {
                    dlclose(handle);
                    continue;
                }

                const clap_plugin_entry_t* entry = entryFn();
                if (!entry || entry->clap_version.major != 1) {
                    dlclose(handle);
                    continue;
                }

                if (!entry->init(pluginPath.c_str())) {
                    dlclose(handle);
                    continue;
                }

                const clap_plugin_factory_t* factory = (const clap_plugin_factory_t*)
                    entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
                if (!factory) {
                    entry->deinit();
                    dlclose(handle);
                    continue;
                }

                uint32_t count = factory->get_plugin_count(factory);
                for (uint32_t i = 0; i < count; ++i) {
                    const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, i);
                    if (!desc) continue;

                    CLAPPluginInfo pluginInfo;
                    pluginInfo.path = pluginPath;
                    pluginInfo.name = desc->name ? desc->name : "Unknown";
                    pluginInfo.id = desc->id ? desc->id : "";
                    pluginInfo.vendor = desc->vendor ? desc->vendor : "";
                    pluginInfo.isInstrument = (desc->features && std::string::npos != std::string(desc->features).find("instrument"));
                    pluginInfo.handle = handle;
                    pluginInfo.factory = factory;
                    pluginInfo.entry = entry;

                    std::lock_guard<std::mutex> lock(data->mutex);
                    data->clapPlugins.push_back(std::move(pluginInfo));
                }
            }
        } catch (const std::exception&) {
            // Ignore errors
        }
    }
}

// JNI native methods

extern "C" {

JNIEXPORT jlong JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTWorld_malloc(JNIEnv* env, jobject obj) {
    return reinterpret_cast<jlong>(new VSTWorldData());
}

JNIEXPORT void JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTWorld_free(JNIEnv* env, jobject obj, jlong instance) {
    VSTWorldData* data = getVSTWorldData(env, instance);
    if (data) {
        // Cleanup CLAP handles
        for (auto& clap : data->clapPlugins) {
            if (clap.entry && clap.handle) {
                clap->deinit();
            }
            if (clap.handle) {
                dlclose(clap.handle);
            }
        }
        delete data;
    }
}

JNIEXPORT jobjectArray JNICALL Java_app_tuxguitar_midi_synth_impl_vst_jni_VSTWorld_getAllPlugins(JNIEnv* env, jobject obj, jlong instance) {
    VSTWorldData* data = getVSTWorldData(env, instance);
    if (!data) return nullptr;

    // Lazy scan on first call
    static std::once_flag scanFlag;
    std::call_once(scanFlag, [data]() {
        scanVST3Plugins(data);
        scanCLAPPlugins(data);
    });

    jclass pluginClass = findClass(env, JNI_CLASS_VSTPLUGIN);
    if (!pluginClass) return nullptr;

    jmethodID pluginCtor = getMethodID(env, pluginClass, "<init>", "(J)V");
    if (!pluginCtor) return nullptr;

    std::lock_guard<std::mutex> lock(data->mutex);
    size_t totalPlugins = data->vst3Plugins.size() + data->clapPlugins.size();
    jobjectArray result = env->NewObjectArray(totalPlugins, pluginClass, nullptr);

    size_t idx = 0;

    // Add VST3 plugins
    for (auto& vst3 : data->vst3Plugins) {
        // We store a pointer to the VST3PluginInfo as the instance
        jlong ptr = reinterpret_cast<jlong>(&vst3);
        jobject pluginObj = env->NewObject(pluginClass, pluginCtor, ptr);
        env->SetObjectArrayElement(result, idx++, pluginObj);
        env->DeleteLocalRef(pluginObj);
    }

    // Add CLAP plugins
    for (auto& clap : data->clapPlugins) {
        jlong ptr = reinterpret_cast<jlong>(&clap);
        jobject pluginObj = env->NewObject(pluginClass, pluginCtor, ptr);
        env->SetObjectArrayElement(result, idx++, pluginObj);
        env->DeleteLocalRef(pluginObj);
    }

    return result;
}

} // extern "C"

// JNI helper functions
static jclass findClass(JNIEnv* env, const char* name) {
    jclass clazz = env->FindClass(name);
    if (!clazz) {
        // Try with class loader
        jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
        jmethodID getSystemClassLoader = env->GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
        jobject classLoader = env->CallStaticObjectMethod(classLoaderClass, getSystemClassLoader);
        jmethodID loadClass = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        jstring className = env->NewStringUTF(name);
        clazz = (jclass)env->CallObjectMethod(classLoader, loadClass, className);
        env->DeleteLocalRef(className);
    }
    return clazz;
}

static jmethodID getMethodID(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    return env->GetMethodID(clazz, name, sig);
}

static jstring toJString(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}