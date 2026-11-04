# CMake script to copy required cmake modules to FluidSynth source directory
# This is executed via PATCH_COMMAND in FetchContent_Declare

cmake_minimum_required(VERSION 3.16)

# Source directory (our local cmake modules)
set(MODULES_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Destination directory (fetched FluidSynth source's cmake_admin)
# This is passed as an argument: -DFLUIDSYNTH_SOURCE_DIR=...
if(NOT FLUIDSYNTH_SOURCE_DIR)
    message(FATAL_ERROR "FLUIDSYNTH_SOURCE_DIR not set")
endif()

set(MODULES_DEST_DIR "${FLUIDSYNTH_SOURCE_DIR}/cmake_admin")

message(STATUS "Copying FluidSynth cmake modules from ${MODULES_SOURCE_DIR} to ${MODULES_DEST_DIR}")

file(MAKE_DIRECTORY "${MODULES_DEST_DIR}")

file(COPY "${MODULES_SOURCE_DIR}/DefaultDirs.cmake" DESTINATION "${MODULES_DEST_DIR}")
file(COPY "${MODULES_SOURCE_DIR}/CheckSTDC.cmake" DESTINATION "${MODULES_DEST_DIR}")
file(COPY "${MODULES_SOURCE_DIR}/TestInline.cmake" DESTINATION "${MODULES_DEST_DIR}")
file(COPY "${MODULES_SOURCE_DIR}/TestVLA.cmake" DESTINATION "${MODULES_DEST_DIR}")
file(COPY "${MODULES_SOURCE_DIR}/UnsetPkgConfig.cmake" DESTINATION "${MODULES_DEST_DIR}")

message(STATUS "FluidSynth cmake modules copied successfully")