cmake_minimum_required(VERSION 3.10)

project(CVDev LANGUAGES CXX)

if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

file(GLOB CPP_FILES *.cpp)

if( WIN32 )
    set(APP_ICON_RESOURCE_WINDOWS "resources/cvdev_resources.rc")
    add_executable(${PROJECT_NAME}
        WIN32
        ${CPP_FILES}
        ${APP_ICON_RESOURCE_WINDOWS}
        resources/cvdev_resources.qrc
        )
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    add_executable(${PROJECT_NAME}
        ${CPP_FILES}
        resources/cvdev_resources.qrc
    )
endif()

add_definitions(-DQT_DEPRECATED_WARNINGS -DNODE_EDITOR_SHARED)

include_directories(${CVDevLibrary_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} CVDevLibrary)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/cvdev_plugins)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/CVDev/log)

# ---- ADD THIS SECTION FOR CPACK ----
# Install the executable target. This tells CPack to include the CVDev
# binary in the package.
# 'RUNTIME DESTINATION bin' installs the executable to the standard '/usr/local/bin'
# directory on the target system.
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)
# Install the desktop entry file for application menus
install(FILES CVDev.desktop
    DESTINATION share/applications
    COMPONENT Runtime
)

# Install the application icon
install(FILES resources/cvdev-64.png
    DESTINATION share/icons/hicolor/scalable/apps
    COMPONENT Runtime
)
# ------------------------------------
