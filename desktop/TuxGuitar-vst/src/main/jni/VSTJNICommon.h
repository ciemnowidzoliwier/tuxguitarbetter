#ifndef VST_JNI_COMMON_H
#define VST_JNI_COMMON_H

#include <string>
#include <vector>

// VST3 SDK forward declarations (avoid including full SDK in header)
namespace Steinberg {
    namespace Vst {
        struct TUID;
        class IComponent;
        class IAudioProcessor;
        class IEditController;
        struct ProcessSetup;
        struct ProcessData;
        struct ParameterInfo;
        using ParamID = int32_t;
    }
}

namespace VST3 {
    namespace Hosting {
        class Module;
        class ClassInfo;
    }
}

// CLAP forward declarations
struct clap_plugin_entry_t;
struct clap_plugin_factory_t;
struct clap_plugin_t;
struct clap_plugin_descriptor_t;
struct clap_param_info_t;

// VST3 Component Info
struct VST3ComponentInfo {
    std::string name;
    std::string id;
    std::string category;
    std::string vendor;
    std::string subCategories;
    bool isInstrument = false;
};

// VST3 Plugin Info
struct VST3PluginInfo {
    std::string path;
    std::vector<VST3ComponentInfo> components;
};

// CLAP Plugin Info
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

// VST World Data
struct VSTWorldData {
    std::vector<VST3PluginInfo> vst3Plugins;
    std::vector<CLAPPluginInfo> clapPlugins;
    std::mutex mutex;
};

// VST Instance Data
struct VSTInstanceData {
    enum Type { NONE, VST3, CLAP } type = NONE;
    union {
        struct VST3InstanceData {
            std::shared_ptr<VST3::Hosting::Module> module;
            Steinberg::Vst::IComponent* component = nullptr;
            Steinberg::Vst::IAudioProcessor* processor = nullptr;
            Steinberg::Vst::IEditController* controller = nullptr;
            Steinberg::Vst::ProcessSetup setup;
            std::vector<Steinberg::Vst::ParamID> paramIds;
        } vst3;
        struct CLAPInstanceData {
            const clap_plugin_entry_t* entry = nullptr;
            const clap_plugin_factory_t* factory = nullptr;
            const clap_plugin_t* plugin = nullptr;
            void* handle = nullptr;
            std::vector<clap_param_info_t> paramInfos;
        } clap;
    };
    std::string pluginPath;
    std::string componentId;
};

// Helper functions
static inline std::string tuidToString(const Steinberg::Vst::TUID& tuid) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%08X%08X%08X%08X",
        tuid[0], tuid[1], tuid[2], tuid[3]);
    return std::string(buffer);
}

static inline bool isInstrumentCategory(const std::string& category) {
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

#endif // VST_JNI_COMMON_H