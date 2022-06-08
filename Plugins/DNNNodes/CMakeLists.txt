cmake_minimum_required(VERSION 3.5)

project(DNNNodes LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
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

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME}
                      PROPERTIES
                      POSITION_INDEPENDENT_CODE ON
                      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/
                      )

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED -DQT_NO_KEYWORDS)

include_directories(${CVDevLibrary_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    CVDevLibrary
    ${OpenCV_LIBS})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E rename
    "$<TARGET_FILE:${PROJECT_NAME}>"
    "${CMAKE_BINARY_DIR}/CVDev/plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    )
