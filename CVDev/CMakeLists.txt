cmake_minimum_required(VERSION 3.5)

project(CVDev LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

file(GLOB CPP_FILES *.cpp)

if (WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "resources/resources.rc")
    add_executable(${PROJECT_NAME}
        WIN32
        ${CPP_FILES}
        ${APP_ICON_RESOURCE_WINDOWS}
        resources/resources.qrc
        )
else()
    add_executable(${PROJECT_NAME}
        ${CPP_FILES})
endif()

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED)

include_directories(${CVDevLibrary_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} CVDevLibrary)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/plugins)
