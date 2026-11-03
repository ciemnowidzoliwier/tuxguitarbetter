cmake_minimum_required(VERSION 3.16)
project(tuxguitar-vst-jni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific library naming
if(WIN32)
    set(JNI_LIB_PREFIX "")
    set(JNI_LIB_SUFFIX ".dll")
elseif(APPLE)
    set(JNI_LIB_PREFIX "lib")
    set(JNI_LIB_SUFFIX ".dylib")
else()
    set(JNI_LIB_PREFIX "lib")
    set(JNI_LIB_SUFFIX ".so")
endif()

# Fetch VST3 SDK
include(FetchContent)
FetchContent_Declare(vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG master
  GIT_SUBMODULES pluginterfaces public.sdk base
)
FetchContent_Populate(vst3sdk)

# Fetch CLAP
FetchContent_Declare(clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG main
)
FetchContent_Populate(clap)

# Source files
file(GLOB VST3_BASE_CPP ${vst3sdk_SOURCE_DIR}/pluginterfaces/base/*.cpp)

set(JNI_SOURCES
  VSTWorld.cpp
  VSTPlugin.cpp
  VSTComponent.cpp
  VSTInstance.cpp
  ${VST3_BASE_CPP}
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module_linux.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/utility/stringconvert.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/common/commonstringconvert.cpp
  ${vst3sdk_SOURCE_DIR}/base/source/fstring.cpp
  ${vst3sdk_SOURCE_DIR}/base/source/fobject.cpp
  ${vst3sdk_SOURCE_DIR}/base/source/baseiids.cpp
  ${vst3sdk_SOURCE_DIR}/base/thread/source/flock.cpp
)

add_library(tuxguitar-vst-jni SHARED ${JNI_SOURCES})

target_include_directories(tuxguitar-vst-jni PRIVATE
  ${vst3sdk_SOURCE_DIR}
  ${clap_SOURCE_DIR}/include
  ${JAVA_INCLUDE_PATH}
  ${JAVA_INCLUDE_PATH2}
)

# SDK requires explicit build mode
target_compile_definitions(tuxguitar-vst-jni PRIVATE RELEASE)

# Platform-specific settings
if(WIN32)
    target_link_libraries(tuxguitar-vst-jni PRIVATE shell32)
    # Static runtime for MinGW cross-compile
    target_link_options(tuxguitar-vst-jni PRIVATE -static -static-libgcc -static-libstdc++)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(tuxguitar-vst-jni PRIVATE dl)
    target_link_options(tuxguitar-vst-jni PRIVATE -static -static-libgcc -static-libstdc++)
elseif(APPLE)
    target_link_libraries(tuxguitar-vst-jni PRIVATE "-framework CoreFoundation")
endif()

# Output name
set_target_properties(tuxguitar-vst-jni PROPERTIES
  OUTPUT_NAME "tuxguitar-vst-jni"
  PREFIX "${JNI_LIB_PREFIX}"
  SUFFIX "${JNI_LIB_SUFFIX}"
)