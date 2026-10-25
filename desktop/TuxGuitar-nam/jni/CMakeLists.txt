cmake_minimum_required(VERSION 3.16)
project(tuxguitar-nam-jni CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(namcore
    GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
    GIT_TAG v0.6.0
)

# Tylko POBIERAMY źródła rdzenia NAM (z submodułami Eigen/nlohmann).
# Celowo NIE wykonujemy jego CMakeLists (add_subdirectory) — buduje swoje
# narzędzia bench/loadmodel, które nie przechodzą na MinGW (M_PI w strict ANSI)
# i niepotrzebnie wydłużają build. Kompilujemy NAM/*.cpp bezpośrednio,
# dokładnie tak samo jak wewnętrzne tools/ rdzenia NAM.
FetchContent_GetProperties(namcore)
if(NOT namcore_POPULATED)
    FetchContent_Populate(namcore)
endif()

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
    ${namcore_SOURCE_DIR}/NAM
    ${namcore_SOURCE_DIR}/Dependencies/eigen
    ${namcore_SOURCE_DIR}/Dependencies/nlohmann
    $ENV{JAVA_HOME}/include
    $ENV{JAVA_HOME}/include/win32
)

target_compile_definitions(tuxguitar-nam-jni PRIVATE
    NAM_SAMPLE_FLOAT
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# Eigen + MinGW: bez tego mogą być crash-e przez niewyrównaną pamięć (Issue 67 w NAM core)
target_compile_options(tuxguitar-nam-jni PRIVATE
    $<$<CONFIG:Release>:-O2>
)
# Windows portable: statycznie dolinkuj runtime MinGW
# (bez tego DLL wymaga libstdc++-6.dll itd., których nie ma w bundlu)
target_link_options(tuxguitar-nam-jni PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
)