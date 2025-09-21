cmake_minimum_required(VERSION 3.10)

project(BasicNodes LANGUAGES CXX)

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

find_package(Qt6 COMPONENTS Widgets OpenGL REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

if( Qt6_FOUND )
    find_package(Qt6 COMPONENTS OpenGLWidgets REQUIRED)
endif()

file(GLOB CPP_FILES *.cpp)
file(GLOB HPP_FILES *.hpp)
file(GLOB UI_FILES *.ui)
add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}
    ${UI_FILES}
    resources/basic_resources.qrc
)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES
                      POSITION_INDEPENDENT_CODE ON
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      )

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED -DQT_NO_KEYWORDS)

include_directories(${CVDevLibrary_INCLUDE_DIRS})

if( Qt6_FOUND )
    target_link_libraries(${PROJECT_NAME}
        CVDevLibrary
        ${OpenCV_LIBS}
        Qt6::OpenGLWidgets
        )
else()
    target_link_libraries(${PROJECT_NAME}
        CVDevLibrary
        ${OpenCV_LIBS}
        )
endif()

# ---- MODIFIED SECTION FOR CPACK ----
# The old 'add_custom_command' has been removed.
#
# This install command places the compiled plugin library (.so file) into a
# dedicated 'plugins' directory for our application.
# 'LIBRARY DESTINATION' is used for shared libraries that are not meant to be
# linked against by other development packages (i.e., plugins).
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION bin/cvdev_plugins
    COMPONENT Runtime
)
# ------------------------------------
