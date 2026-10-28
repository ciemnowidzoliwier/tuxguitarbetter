cmake_minimum_required(VERSION 3.16)
project(tuxguitar-vst-spike LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# VST3 SDK (MIT, v3.8.x) - pobieramy submoduly pluginterfaces/public.sdk/base.
# UWAGA: celowo NIE add_subdirectory - oficjalny CMake SDK celuje w MSVC.
# Kompilujemy wylacznie podzbior hostingowy.
FetchContent_Declare(vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG master
  GIT_SUBMODULES pluginterfaces public.sdk base
)
FetchContent_Populate(vst3sdk)

# CLAP (MIT) - czyste naglowki C, nic nie kompilujemy.
FetchContent_Declare(clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG main
)
FetchContent_Populate(clap)

file(GLOB VST3_BASE_CPP ${vst3sdk_SOURCE_DIR}/pluginterfaces/base/*.cpp)

add_executable(vst_spike
  spike_main.cpp
  ${VST3_BASE_CPP}
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module_win32.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/utility/stringconvert.cpp
  # implementacja Steinberg::StringConvert (link error: undefined reference to
  # Steinberg::StringConvert::convert) - deklaracja w commonstringconvert.h:
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/common/commonstringconvert.cpp
  # implementacja Steinberg::String (uzywana przez module_win32) - z submodulu base:
  ${vst3sdk_SOURCE_DIR}/base/source/fstring.cpp
  # FObject (baza dla StringObject w fstring.cpp) + iid interfejsow base:
  ${vst3sdk_SOURCE_DIR}/base/source/fobject.cpp
  ${vst3sdk_SOURCE_DIR}/base/source/baseiids.cpp
  # FLock uzywany przez FObject (Steinberg::Base::Thread):
  ${vst3sdk_SOURCE_DIR}/base/thread/source/flock.cpp
)

target_include_directories(vst_spike PRIVATE
  ${vst3sdk_SOURCE_DIR}
  ${clap_SOURCE_DIR}/include
)

# SDK wymaga jawnego trybu (fdebug.h: #error DEVELOPMENT, RELEASE, _DEBUG, or NDEBUG
# must be defined!) - oficjalny CMake SDK ustawia to sam, my kompilujemy recznie.
target_compile_definitions(vst_spike PRIVATE RELEASE)

# SDK linkuje Shell32 przez #pragma comment (ignorowane przez GCC) - linkuj jawnie.
target_link_libraries(vst_spike PRIVATE shell32)

# Statyczny runtime jak w TG-NAM (bez zewnetrznych zaleznosci MinGW).
target_link_options(vst_spike PRIVATE -static -static-libgcc -static-libstdc++)
