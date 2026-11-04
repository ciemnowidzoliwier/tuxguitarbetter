cmake_minimum_required(VERSION 3.16)
project(tuxguitar-fluidsynth-jni CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Android NDK toolchain is set by Gradle via -DCMAKE_TOOLCHAIN_FILE

include(FetchContent)
FetchContent_Declare(fluidsynth
    GIT_REPOSITORY https://github.com/FluidSynth/fluidsynth.git
    GIT_TAG v2.3.4
    # Patch to disable mandatory GLib2 find_package for Android cross-compilation
    PATCH_COMMAND sed -i 's/find_package ( GLib2/# find_package ( GLib2/' <SOURCE_DIR>/CMakeLists.txt
)

FetchContent_GetProperties(fluidsynth)
if(NOT fluidsynth_POPULATED)
    FetchContent_Populate(fluidsynth)
endif()

# Configure FluidSynth build options for Android
# Must be set BEFORE add_subdirectory
set(FLUIDSYNTH_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(LIB_SUFFIX "" CACHE STRING "" FORCE)
set(enable-ladspa OFF CACHE BOOL "" FORCE)
set(enable-dbus OFF CACHE BOOL "" FORCE)
set(enable-portaudio OFF CACHE BOOL "" FORCE)
set(enable-jack OFF CACHE BOOL "" FORCE)
set(enable-pulseaudio OFF CACHE BOOL "" FORCE)
set(enable-alsa OFF CACHE BOOL "" FORCE)
set(enable-oss OFF CACHE BOOL "" FORCE)
set(enable-coreaudio OFF CACHE BOOL "" FORCE)
set(enable-winmidi OFF CACHE BOOL "" FORCE)
set(enable-readline OFF CACHE BOOL "" FORCE)
set(enable-waveout OFF CACHE BOOL "" FORCE)
set(enable-dsound OFF CACHE BOOL "" FORCE)
set(enable-midishare OFF CACHE BOOL "" FORCE)
# Disable GLib2 - not available for Android cross-compilation
set(enable-glib2 OFF CACHE BOOL "" FORCE)
set(FLUIDSYNTH_ENABLE_GLIB2 OFF CACHE BOOL "" FORCE)
set(enable_glib2 OFF CACHE BOOL "" FORCE)
set(WITH_GLIB2 OFF CACHE BOOL "" FORCE)
set(ENABLE_GLIB2 OFF CACHE BOOL "" FORCE)
set(GLIB2_FOUND OFF CACHE BOOL "" FORCE)
set(GLib2_FOUND OFF CACHE BOOL "" FORCE)

# Build FluidSynth as a static library to embed in our JNI library
# This avoids needing to package libfluidsynth.so separately
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${fluidsynth_SOURCE_DIR} ${fluidsynth_BINARY_DIR})

# Now create our JNI wrapper
add_library(tuxguitar-fluidsynth-jni SHARED
    tuxguitar_fluidsynth_jni.cpp
)

target_include_directories(tuxguitar-fluidsynth-jni PRIVATE
    ${fluidsynth_SOURCE_DIR}/src/include
    ${fluidsynth_BINARY_DIR}/src/include
)

# Link against FluidSynth static library
target_link_libraries(tuxguitar-fluidsynth-jni PRIVATE
    fluidsynth
    log
    OpenSLES
)

set_target_properties(tuxguitar-fluidsynth-jni PROPERTIES
    PREFIX ""
    OUTPUT_NAME "tuxguitar-fluidsynth-jni"
)

target_compile_definitions(tuxguitar-fluidsynth-jni PRIVATE
    NOMINMAX
)

target_compile_options(tuxguitar-fluidsynth-jni PRIVATE
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Release>:-DNDEBUG>
)