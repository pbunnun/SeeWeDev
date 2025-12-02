cmake_minimum_required(VERSION 3.10)

project(DNNNodes LANGUAGES CXX)

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
file(GLOB UI_FILES *.ui)
add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}
    ${UI_FILES}
    resources/dnn_resources.qrc
)

# ---- MODIFIED THIS SECTION ----
# Replaced RUNTIME_OUTPUT_DIRECTORY with LIBRARY_OUTPUT_DIRECTORY to ensure
# the plugin is built into the correct subfolder for local execution.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES
                      POSITION_INDEPENDENT_CODE ON
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/
                      )
# -----------------------------

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED -DQT_NO_KEYWORDS)

include_directories(${CVDevLibrary_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    CVDevLibrary
    ${OpenCV_LIBS}
)

if(WIN32)
    # Move the built runtime DLL from CVDev -> CVDev/cvdev_plugins.
    # Assumes CVDev/cvdev_plugins already exists and the runtime DLL is placed in CVDev.
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Moving $<TARGET_FILE_NAME:${PROJECT_NAME}> to ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins"
        COMMAND ${CMAKE_COMMAND} -E rename
                "${CMAKE_BINARY_DIR}/CVDev/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
                "${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Move ${PROJECT_NAME} into cvdev_plugins"
        VERBATIM)
endif()

# ---- MODIFIED SECTION FOR CPACK ----
# The old 'add_custom_command' has been removed.
#
# This install command places the compiled plugin library (.so file) into the
# application's dedicated plugins directory upon installation.
if( NOT WIN32 )
    install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION bin/cvdev_plugins
        COMPONENT Runtime
    )
endif()
# ------------------------------------
