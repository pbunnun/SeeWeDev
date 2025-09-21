cmake_minimum_required(VERSION 3.10)

project(NodeEditor LANGUAGES CXX)

if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE Release)
endif()

if( NOT WIN32 )
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets OpenGL REQUIRED)

file(GLOB CPP_FILES src/*.cpp)
file(GLOB HPP_FILES src/*.hpp include/nodes/internal/*.hpp include/nodes/*)
add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}
    
    resources/resources.qrc
)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES 
                      POSITION_INDEPENDENT_CODE ON
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/)

include_directories(include/nodes/internal)

add_definitions(-DNODE_EDITOR_EXPORTS -DNODE_EDITOR_SHARED -DQT_NO_KEYWORDS -Dnodes_EXPORTS)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::OpenGL)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/nodes
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

# ---- ADD THIS SECTION FOR CPACK ----
# Install the library target and its public headers.

# 1. Install the shared library (.so file).
#    This places the library in a standard system directory.
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    COMPONENT Runtime
)

# 2. Install the public header files from the 'include' directory.
#    'DIRECTORY include/' installs the entire directory.
#    This preserves the subdirectory structure (like 'nodes/'), so that
#    #include <nodes/Node.hpp> works correctly.
install(DIRECTORY include/
    DESTINATION include/${PROJECT_NAME}
    COMPONENT Development
)
# ------------------------------------
