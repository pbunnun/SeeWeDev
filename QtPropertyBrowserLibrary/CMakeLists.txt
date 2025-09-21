cmake_minimum_required(VERSION 3.10)

project(QtPropertyBrowserLibrary LANGUAGES CXX)

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

file(GLOB CPP_FILES qtpropertybrowser/*.cpp)
file(GLOB HPP_FILES qtpropertybrowser/*.h *.hpp)
add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}

    qtpropertybrowser/qtpropertybrowser.qrc
)

add_definitions(-DQT_DEPRECATED_WARNINGS -DQTPROPERTYBROWSER_LIBRARY)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES 
                      POSITION_INDEPENDENT_CODE ON
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/)

target_link_libraries(${PROJECT_NAME} Qt6::Widgets)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/qtpropertybrowser
    ${PROJECT_SOURCE_DIR}
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

# ---- ADD THIS SECTION FOR CPACK ----
# Install the library target and its public headers.

# 1. Install the shared library (.so file).
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    COMPONENT Runtime
)

# 2. Install the public header files from the 'qtpropertybrowser' directory.
install(FILES ${HPP_FILES}
    DESTINATION include/${PROJECT_NAME}
    COMPONENT Development
)
# ------------------------------------
