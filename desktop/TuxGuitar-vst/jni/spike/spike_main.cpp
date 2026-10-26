// Spike S0 - TG-VST: weryfikacja, ze podzbior hostingowy VST3 SDK + CLAP
// kompiluje sie i linkuje na MinGW (cross-compile z Linuksa na Windows x64).
//
// Listuje pluginy VST3 i CLAP z katalogow systemowych:
//   VST3: skan known-folders przez VST3::Hosting::Module::getModulePaths()
//         (lub katalog z env SPIKE_VST3_DIR)
//   CLAP: %COMMONPROGRAMFILES%\CLAP + katalogi z env CLAP_PATH (';'-oddzielone)
// Argument: vst_spike.exe <plik.vst3 | plik.clap>  -> tylko ten plik.

#include <windows.h>

#include <clap/entry.h>
#include <clap/factory/plugin-factory.h>

#include "public.sdk/source/vst/hosting/module.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

//------------------------------------------------------------------------
static void listVst3Module(const std::string& path)
{
    std::string error;
    auto module = VST3::Hosting::Module::create(path, error);
    if (!module)
    {
        std::cout << "  [BLAD] " << error << std::endl;
        return;
    }
    std::cout << "  modul: " << module->getName() << std::endl;
    const auto infos = module->getFactory().classInfos();
    std::cout << "  klas: " << infos.size() << std::endl;
    for (const auto& info : infos)
    {
        std::cout << "   - " << info.name()
                  << " | kategoria: " << info.category()
                  << " | vendor: " << info.vendor()
                  << " | sub: " << info.subCategoriesString()
                  << " | id: " << info.ID().toString() << std::endl;
    }
}

//------------------------------------------------------------------------
static void listClapFile(const std::string& path)
{
    HMODULE lib = LoadLibraryA(path.c_str());
    if (!lib)
    {
        std::cout << "  [BLAD] LoadLibrary, kod: " << GetLastError() << std::endl;
        return;
    }
    auto entryFn = reinterpret_cast<const clap_plugin_entry_t* (*)()>(
        reinterpret_cast<void*>(GetProcAddress(lib, "clap_entry")));
    if (!entryFn)
    {
        std::cout << "  [BLAD] brak exportu 'clap_entry'" << std::endl;
        FreeLibrary(lib);
        return;
    }
    const clap_plugin_entry_t* entry = entryFn();
    if (entry->clap_version.major != 1)
    {
        std::cout << "  [BLAD] nieobslugiwana wersja CLAP" << std::endl;
        FreeLibrary(lib);
        return;
    }
    if (!entry->init(path.c_str()))
    {
        std::cout << "  [BLAD] init() zwrocil false" << std::endl;
        FreeLibrary(lib);
        return;
    }
    const auto factory = static_cast<const clap_plugin_factory_t*>(
        entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if (factory)
    {
        const uint32_t count = factory->get_plugin_count(factory);
        std::cout << "  pluginow: " << count << std::endl;
        for (uint32_t i = 0; i < count; ++i)
        {
            const clap_plugin_descriptor_t* d = factory->get_plugin_descriptor(factory, i);
            if (!d)
                continue;
            std::cout << "   - " << (d->name ? d->name : "?")
                      << " | id: " << (d->id ? d->id : "?")
                      << " | vendor: " << (d->vendor ? d->vendor : "?")
                      << std::endl;
        }
    }
    else
    {
        std::cout << "  [BLAD] brak fabryki plugin-factory" << std::endl;
    }
    entry->deinit();
    FreeLibrary(lib);
}

//------------------------------------------------------------------------
static void scanDirForExt(const fs::path& dir, const std::string& ext,
                          std::vector<fs::path>& out)
{
    std::error_code ec;
    if (!fs::exists(dir, ec))
        return;
    for (auto it = fs::recursive_directory_iterator(
             dir, fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
    {
        std::error_code ec2;
        if (!it->is_regular_file(ec2) || ec2)
            continue;
        if (it->path().extension() == ext)
            out.push_back(it->path());
    }
    if (ec)
        std::cout << "  [UWAGA] skan " << dir << ": " << ec.message() << std::endl;
}

//------------------------------------------------------------------------
int main(int argc, char** argv)
{
    std::cout << "[TG-VST SPIKE] start" << std::endl;

    try
    {
        if (argc > 1)
        {
            const fs::path p(argv[1]);
            if (p.extension() == ".vst3")
                listVst3Module(argv[1]);
            else if (p.extension() == ".clap")
                listClapFile(argv[1]);
            else
                std::cout << "Podaj plik .vst3 lub .clap" << std::endl;
            return 0;
        }

        std::cout << "\n=== VST3 ===" << std::endl;
        std::vector<std::string> vst3Paths;
        if (const char* vstDir = std::getenv("SPIKE_VST3_DIR"))
        {
            std::vector<fs::path> files;
            scanDirForExt(fs::path(vstDir), ".vst3", files);
            for (const auto& f : files)
                vst3Paths.push_back(f.string());
        }
        else
        {
            vst3Paths = VST3::Hosting::Module::getModulePaths();
        }
        std::cout << "modulow VST3: " << vst3Paths.size() << std::endl;
        for (const auto& p : vst3Paths)
        {
            std::cout << " " << p << std::endl;
            listVst3Module(p);
        }

        std::cout << "\n=== CLAP ===" << std::endl;
        std::vector<fs::path> clapFiles;
        if (const char* common = std::getenv("COMMONPROGRAMFILES"))
            scanDirForExt(fs::path(common) / "CLAP", ".clap", clapFiles);
        if (const char* clapPathEnv = std::getenv("CLAP_PATH"))
        {
            const std::string s(clapPathEnv);
            size_t start = 0;
            while (start <= s.size())
            {
                const size_t end = s.find(';', start);
                const std::string tok =
                    s.substr(start, (end == std::string::npos ? s.size() : end) - start);
                if (!tok.empty())
                    scanDirForExt(fs::path(tok), ".clap", clapFiles);
                if (end == std::string::npos)
                    break;
                start = end + 1;
            }
        }
        std::cout << "plikow .clap: " << clapFiles.size() << std::endl;
        for (const auto& p : clapFiles)
        {
            std::cout << " " << p.string() << std::endl;
            listClapFile(p.string());
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "[TG-VST SPIKE] wyjatek: " << e.what() << std::endl;
    }

    std::cout << "\n[TG-VST SPIKE] koniec - kompilacja, linkowanie i start OK" << std::endl;
    return 0;
}
