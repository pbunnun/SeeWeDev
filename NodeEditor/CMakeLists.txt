cmake_minimum_required(VERSION 3.5)

project(NodeEditor LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets OpenGL REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets OpenGL REQUIRED)

file(GLOB CPP_FILES src/*.cpp)
file(GLOB HPP_FILES src/*.hpp include/nodes/internal/*.hpp)
add_library(${PROJECT_NAME} SHARED
    ${CPP_FILES}
    ${HPP_FILES}
    include/nodes/Connection
    include/nodes/ConnectionStyle 
    include/nodes/DataModelRegistry 
    include/nodes/FlowScene 
    include/nodes/FlowView 
    include/nodes/FlowViewStyle 
    include/nodes/Node 
    include/nodes/NodeData 
    include/nodes/NodeDataModel 
    include/nodes/NodeGeometry 
    include/nodes/NodePainterDelegate 
    include/nodes/NodeState 
    include/nodes/NodeStyle 
    include/nodes/TypeConverter

    resources/resources.qrc
)

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES POSITION_INDEPENDENT_CODE ON
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/)

include_directories(include/nodes/internal)

add_definitions(-DNODE_EDITOR_EXPORTS -DNODE_EDITOR_SHARED -DQT_NO_KEYWORDS -Dnodes_EXPORTS)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/nodes
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
