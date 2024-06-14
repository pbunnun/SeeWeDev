cmake_minimum_required(VERSION 3.5)

project(Plugins)

set(GFLAGS_USE_TARGET_NAMESPACE ON)

add_subdirectory(BasicNodes)
add_subdirectory(DNNNodes)
