cmake_minimum_required(VERSION 3.10)

project(CVDevLibrary LANGUAGES C CXX)

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

# Try to find Zenoh (optional dependency for distributed mode)
find_package(zenohc QUIET)
if(zenohc_FOUND)
    message(STATUS "Zenoh found - enabling distributed dataflow support")
    add_definitions(-DZENOH_ENABLED)
    set(ZENOH_LIBRARIES zenohc::lib)
else()
    message(STATUS "Zenoh not found - distributed dataflow disabled (Qt-only mode)")
    set(ZENOH_LIBRARIES "")
endif()

# Try to find CycloneDDS (optional dependency for DDS transport mode)
find_path(CYCLONEDDS_INCLUDE_DIR
    NAMES dds/dds.h
    PATHS /usr/local/include /opt/homebrew/include
)
find_library(CYCLONEDDS_LIBRARY
    NAMES ddsc
    PATHS /usr/local/lib /opt/homebrew/lib
)

find_program(CYCLONEDDS_IDLC_EXECUTABLE
    NAMES idlc
    PATHS /usr/local/bin /opt/homebrew/bin
)

if(CYCLONEDDS_INCLUDE_DIR AND CYCLONEDDS_LIBRARY AND CYCLONEDDS_IDLC_EXECUTABLE)
    message(STATUS "CycloneDDS found - enabling DDS transport support")
    add_definitions(-DCYCLONEDDS_ENABLED)
    set(CYCLONEDDS_LIBRARIES ${CYCLONEDDS_LIBRARY})
    set(CYCLONEDDS_IDL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/cyclonedds_idl)
    set(CYCLONEDDS_IDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/CycloneDDSRawPayload.idl)
    set(CYCLONEDDS_IDL_C ${CYCLONEDDS_IDL_OUTPUT_DIR}/CycloneDDSRawPayload.c)
    set(CYCLONEDDS_IDL_H ${CYCLONEDDS_IDL_OUTPUT_DIR}/CycloneDDSRawPayload.h)

    file(MAKE_DIRECTORY ${CYCLONEDDS_IDL_OUTPUT_DIR})

    add_custom_command(
        OUTPUT ${CYCLONEDDS_IDL_C} ${CYCLONEDDS_IDL_H}
        COMMAND ${CYCLONEDDS_IDLC_EXECUTABLE} -l c -o ${CYCLONEDDS_IDL_OUTPUT_DIR} ${CYCLONEDDS_IDL_FILE}
        DEPENDS ${CYCLONEDDS_IDL_FILE}
        COMMENT "Generating CycloneDDS raw payload type from IDL"
        VERBATIM
    )

    set(CYCLONEDDS_GENERATED_SOURCES ${CYCLONEDDS_IDL_C})
else()
    message(STATUS "CycloneDDS not found (or idlc missing) - DDS transport disabled")
    set(CYCLONEDDS_LIBRARIES "")
    set(CYCLONEDDS_GENERATED_SOURCES "")
endif()

file(GLOB CPP_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB HPP_FILES CONFIGURE_DEPENDS *.hpp)

add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${CYCLONEDDS_GENERATED_SOURCES}
    ${HPP_FILES}
    MainWindow.ui

    resources/cvdevlibrary_resources.qrc
)

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED -DCVDEV_LIBRARY)

include_directories(
    ${CMAKE_SOURCE_DIR}/NodeEditor/include
    ${QtPropertyBrowserLibrary_INCLUDE_DIRS}
)

if(CYCLONEDDS_INCLUDE_DIR AND CYCLONEDDS_LIBRARY AND CYCLONEDDS_IDLC_EXECUTABLE)
    include_directories(
        ${CYCLONEDDS_INCLUDE_DIR}
        ${CYCLONEDDS_IDL_OUTPUT_DIR}
    )
endif()


# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES 
                      POSITION_INDEPENDENT_CODE ON
                      # Multi-config generators (Visual Studio) put outputs in config subdirs
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/
                      LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/CVDev/
                      ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/CVDev/
                      )

target_link_libraries(${PROJECT_NAME}
    QtNodes
    QtPropertyBrowserLibrary
    ${OpenCV_LIBS}
    ${ZENOH_LIBRARIES}
    ${CYCLONEDDS_LIBRARIES}
    )

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/NodeEditor/include
    ${QtPropertyBrowserLibrary_INCLUDE_DIRS}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

# ---- ADD THIS SECTION FOR CPACK ----
# Install the library target and its public headers.

if(NOT WIN32)
    # 1. Install the shared library (.so file) and archive file (.a file).
    #    'LIBRARY DESTINATION lib' installs shared libraries to /usr/local/lib.
    #    'ARCHIVE DESTINATION lib' installs static libraries to /usr/local/lib.
    install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        COMPONENT Runtime
    )
endif()

# 2. Install the public header files (.hpp files).
#    This makes them available for other projects that might link against this library.
#    'DESTINATION include/${PROJECT_NAME}' organizes them neatly in a subdirectory,
#    e.g., /usr/local/include/CVDevLibrary/
install(FILES ${HPP_FILES}
    DESTINATION include/${PROJECT_NAME}
    COMPONENT Development
)
# ------------------------------------
