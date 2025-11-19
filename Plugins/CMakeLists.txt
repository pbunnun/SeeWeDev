cmake_minimum_required(VERSION 3.10)

project(Plugins)

set(GFLAGS_USE_TARGET_NAMESPACE ON)

# Make centralized plugin tools available to all plugins
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Common)

add_subdirectory(BasicNodes)
add_subdirectory(DNNNodes)
add_subdirectory(GUINodes)
