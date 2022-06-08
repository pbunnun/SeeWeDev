cmake_minimum_required(VERSION 3.5)

project(CVDevLibrary LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

file(GLOB CPP_FILES *.cpp)
file(GLOB HPP_FILES *.hpp)

add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}
    MainWindow.ui

    resources/resources.qrc
)

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED -DCVDEV_LIBRARY)

include_directories(${NodeEditor_INCLUDE_DIRS} ${QtPropertyBrowserLibrary_INCLUDE_DIRS})

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES POSITION_INDEPENDENT_CODE ON
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/)

target_link_libraries(${PROJECT_NAME}
    NodeEditor
    QtPropertyBrowserLibrary)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}
    ${NodeEditor_INCLUDE_DIRS}
    ${QtPropertyBrowserLibrary_INCLUDE_DIRS}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
