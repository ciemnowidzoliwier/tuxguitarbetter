cmake_minimum_required(VERSION 3.16)
project(tuxguitar-nam-jni CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Android NDK toolchain is set by Gradle via -DCMAKE_TOOLCHAIN_FILE
# We just need to ensure we use the right STL

include(FetchContent)
FetchContent_Declare(namcore
    GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
    GIT_TAG v0.6.0
)

FetchContent_GetProperties(namcore)
if(NOT namcore_POPULATED)
    FetchContent_Populate(namcore)
endif()

# NAM sources - same as desktop
file(GLOB NAM_SOURCES_TOP "${namcore_SOURCE_DIR}/NAM/*.cpp" "${namcore_SOURCE_DIR}/NAM/*.c")
file(GLOB NAM_SOURCES_SUB "${namcore_SOURCE_DIR}/NAM/*/*.cpp")
set(NAM_SOURCES ${NAM_SOURCES_TOP} ${NAM_SOURCES_SUB})

add_library(tuxguitar-nam-jni SHARED
    tuxguitar_nam_jni.cpp
    ${NAM_SOURCES}
)

set_target_properties(tuxguitar-nam-jni PROPERTIES
    PREFIX ""
    OUTPUT_NAME "tuxguitar-nam-jni"
)

target_include_directories(tuxguitar-nam-jni PRIVATE
    ${namcore_SOURCE_DIR}
    ${namcore_SOURCE_DIR}/NAM
    ${namcore_SOURCE_DIR}/Dependencies/eigen
    ${namcore_SOURCE_DIR}/Dependencies/nlohmann
)

target_compile_definitions(tuxguitar-nam-jni PRIVATE
    NAM_SAMPLE_FLOAT
    NOMINMAX
)

# Android: use c++_shared STL (set via Gradle arguments)
# No static linking needed on Android - c++_shared is loaded by app

# Optimization
target_compile_options(tuxguitar-nam-jni PRIVATE
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Release>:-DNDEBUG>
)

# Link against log for Android logging
target_link_libraries(tuxguitar-nam-jni PRIVATE log)