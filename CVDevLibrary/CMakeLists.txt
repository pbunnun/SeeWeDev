cmake_minimum_required(VERSION 3.10)

project(CVDevLibrary LANGUAGES CXX)

if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE Release)
endif()

if( NOT WIN32 )
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

file(GLOB CPP_FILES *.cpp)
file(GLOB HPP_FILES *.hpp)

add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}
    MainWindow.ui

    resources/cvdevlibrary_resources.qrc
)

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED -DCVDEV_LIBRARY)

include_directories(
    ${CMAKE_SOURCE_DIR}/NodeEditor/include
    ${QtPropertyBrowserLibrary_INCLUDE_DIRS}
)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES 
                      POSITION_INDEPENDENT_CODE ON
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/)

target_link_libraries(${PROJECT_NAME}
    QtNodes
    QtPropertyBrowserLibrary
    ${OpenCV_LIBS}
    )

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/NodeEditor/include
    ${QtPropertyBrowserLibrary_INCLUDE_DIRS}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

# ---- ADD THIS SECTION FOR CPACK ----
# Install the library target and its public headers.

# 1. Install the shared library (.so file) and archive file (.a file).
#    'LIBRARY DESTINATION lib' installs shared libraries to /usr/local/lib.
#    'ARCHIVE DESTINATION lib' installs static libraries to /usr/local/lib.
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    COMPONENT Runtime
)

# 2. Install the public header files (.hpp files).
#    This makes them available for other projects that might link against this library.
#    'DESTINATION include/${PROJECT_NAME}' organizes them neatly in a subdirectory,
#    e.g., /usr/local/include/CVDevLibrary/
install(FILES ${HPP_FILES}
    DESTINATION include/${PROJECT_NAME}
    COMPONENT Development
)
# ------------------------------------
