cmake_minimum_required(VERSION 3.16)
project(tuxguitar-fluidsynth-jni CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Android NDK toolchain is set by Gradle via -DCMAKE_TOOLCHAIN_FILE

include(FetchContent)
# Use FluidSynth v1.1.11 (last v1.x release) which doesn't require GLib2
# v2.0.0+ made GLib2 a mandatory dependency which is problematic for Android cross-compilation
FetchContent_Declare(fluidsynth
    GIT_REPOSITORY https://github.com/FluidSynth/fluidsynth.git
    GIT_TAG v1.1.11
    GIT_SHALLOW TRUE
    # Copy our local cmake modules to fetched source's cmake_admin directory
    # This ensures the modules are available when the fetched source's CMakeLists.txt runs
    # Use a CMake script to avoid shell escaping issues in ninja build files
    PATCH_COMMAND
        ${CMAKE_COMMAND}
        -DFLUIDSYNTH_SOURCE_DIR=${fluidsynth_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_fluidsynth.cmake
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
# Enable OpenSLES for Android audio output
set(enable-opensles ON CACHE BOOL "" FORCE)

# Build FluidSynth as a static library to embed in our JNI library
# This avoids needing to package libfluidsynth.so separately
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${fluidsynth_SOURCE_DIR} ${fluidsynth_BINARY_DIR})

# Now create our JNI wrapper
add_library(tuxguitar-fluidsynth-jni SHARED
    tuxguitar_fluidsynth_jni.cpp
)

target_include_directories(tuxguitar-fluidsynth-jni PRIVATE
    ${fluidsynth_SOURCE_DIR}/include
    ${fluidsynth_BINARY_DIR}/include
)

# Link against FluidSynth static library (libfluidsynth.a)
# The 'fluidsynth' target might be the executable, so use the library file directly
target_link_libraries(tuxguitar-fluidsynth-jni PRIVATE
    ${fluidsynth_BINARY_DIR}/src/libfluidsynth.a
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